/*
 * I Changed m5Stack/M5Stack Example(https://github.com/m5stack/M5Stack/blob/master/examples/Unit/UWB_DW1000/UWB_DW1000.ino)
 * for M5Core2
 * UWB Module use UART Communication, but m5core2 only have external pin for I2C
 */

/*
    Description: UWB Unit test program. When using, two M5 devices are burned to the program, 
    and the buttons A and B are used to configure them to base station mode and Tag mode respectively. 
    For more details, please refer to the code comments below.
    Before use, it is recommended to configure the base station first, and then configure the Tag
*/

/*
 * Position of anchors
 * Base 0 : (0, 0)
 * Base 1 : (x, 0)
 * Base 2 : (x, y)
 * Base 3 : (0, y)
 */

#include <M5Core2.h>
#include <math.h>
// Please input the location of A
double anchorX = 1;
double anchorY = 1;
const double PI = 3.14159265358979;
const int numOfAnchor = 4;

String DATA = " ";  //Used to store distance data
int UWB_MODE = 2;   //Used to set UWB mode

int UWB_T_UI_NUMBER_2 = 0; // flag bit 标志位
int UWB_T_UI_NUMBER_1 = 0;
int UWB_T_NUMBER = 0;
int UWB_B_NUMBER = 0;

hw_timer_t *timer = NULL;
int timer_flag = 0;
int base_flag = 0;
uint32_t timer_data = 0;
static void IRAM_ATTR Timer0_CallBack(void);

// Calculation Trilateration
// numbering is anchor id
// T mean is Tag
// H mean is foot of perpendicular of T
void Trilateration() {
  double disNtoT[4], angleTnH[4];
  double xn[4], yn[4];
  double x=0, y=0;

  // temp
  disNtoT[0] = 0.7;
  disNtoT[1] = 0.9;
  disNtoT[2] = 0.93;
  disNtoT[3] = 0.9;

  for (int i = 0; i < numOfAnchor; i++) {
    int j = i % 2 ? i-1 : i+1;
    // A = arccos((b^2 + c^2 - a^2)/(2bc))
    angleTnH[i] = acos((pow(anchorX, 2) + pow(disNtoT[i], 2) - pow(disNtoT[j], 2)) / (2 * anchorX * disNtoT[i]));
    xn[i] = cos(angleTnH[i]) * disNtoT[i];
    yn[i] = sin(angleTnH[i]) * disNtoT[i];
    x += xn[i];
    y += yn[i];
  }
  x /= numOfAnchor;
  y /= numOfAnchor;
}

//Data display
void UWB_display()
{
  switch (UWB_MODE)
  {
    case 0:

      if (UWB_T_NUMBER > 0 && UWB_T_NUMBER < 5)
      {
        int c = UWB_T_NUMBER;
        int b = 4 - UWB_T_NUMBER;
        while (c > 0)
        {
          c--;
          M5.Lcd.drawString(DATA.substring(2 + c * 11, 3 + c * 11), 37, 50 + c * 40, 4 );//Tag the serial number 标签序号
          M5.Lcd.drawString(DATA.substring(4 + c * 11, 8 + c * 11), 210, 50 + c * 40, 4); //Distance 距离
        }
        while (b > 0)
        {
          b--;
          M5.Lcd.fillRect(0, 170 - b * 40, 340, 30, BLACK);
        }
      }
      break;
    case 1:
      if (UWB_B_NUMBER == 1)
      {
        M5.Lcd.drawString(DATA, 60, 90, 4);//Prompt information  (Base station 0 only) 提示信息 (仅限基站0)
      }
      break;
  }
}

//UI display
void UWB_ui_display() {

  M5.Lcd.drawString("UWB Example", 90, 0, 4); //UI
  M5.Lcd.drawString("Tag", 50, 210, 4);
  M5.Lcd.drawString("Base", 130, 210, 4);
  M5.Lcd.drawString("Reset", 220, 210, 4);

  switch (UWB_MODE)
  {
    case 0:
      if (UWB_T_NUMBER > 0 && UWB_T_NUMBER < 5)   //Tag mode UI display  标签模式UI显示
      {
        int c = UWB_T_NUMBER;
        int b = 4 - UWB_T_NUMBER;
        while (c > 0 )
        {
          c--;
          M5.Lcd.drawString("B", 20, 50 + c * 40, 4 );
          M5.Lcd.drawString("Distance:", 80, 50 + c * 40, 4 );
          M5.Lcd.drawString("M", 280, 50 + c * 40, 4);
        }

        while (b > 0)
        {
          b--;
          M5.Lcd.fillRect(0, 170 - b * 40, 340, 30, BLACK);
        }
      }
      break;
    case 1:                                              //Base station mode UI display  基站模式UI显示
      M5.Lcd.drawString("Base station", 80, 50, 4 );
      M5.Lcd.drawString(String(UWB_B_NUMBER), 240, 50, 4 );//UWB_B_NUMBER
      if (UWB_B_NUMBER == 0)
      {
        M5.Lcd.drawString("loading......", 100, 90, 4 );
      }
      else
      {
        M5.Lcd.fillRect(0, 90, 340, 30, BLACK);
      }

      break;
  }
}
//  Display and data clear
void UWB_clear() {               //Clear the display and DATA  清除显示和DATA
  if (Serial2.available())
  {
    delay(3);
    DATA = Serial2.readString();
  }
  DATA = "";
  timer_flag = 0;
  timer_data = 0;
  M5.Lcd.fillRect(0, 50, 340, 150, BLACK);
}

int UWB_readString()          //Read UART data  读取串口数据
{
  switch (UWB_MODE)
  {
    case 0:
      if (Serial2.available())
      {
        delay(20);
        UWB_T_NUMBER = (Serial2.available() / 11);      //Count the number of Base stations  计算基站数目
        delay(20);
        if (UWB_T_NUMBER != UWB_T_UI_NUMBER_1 || UWB_T_UI_NUMBER_2 == 0)    //Changing the UI display  更改UI显示
        {
          UWB_ui_display();
          UWB_T_UI_NUMBER_1 = UWB_T_NUMBER;
          UWB_T_UI_NUMBER_2 = 1;
        }
        DATA = Serial2.readString();
        delay(2);
        timer_flag = 0;
        timer_data = 1;
        break;
      }
      else
      {
        timer_flag = 1;
      }
      if (timer_data == 0 || timer_data > 8)    //Count the number of Base stations  提示与基站0断连（测试）
      {
        if(timer_data==9){M5.Lcd.fillRect(210, 50, 50, 30, BLACK);}
        DATA = "  0 2F   ";
        timer_flag = 0;
      }
      break;
    case 1:
      if (timer_data == 0 || timer_data > 70)   //Indicates successful or lost connection with Tag  提示与标签连接成功或丢失断连
      {
        if (Serial2.available())
        {
          delay(2);
          DATA = Serial2.readString();
          DATA = "set up successfully!";
          timer_data = 1;
          timer_flag = 1;
          break;
        }
        else if (timer_data > 0 && Serial2.available() == 0 )
        {
          DATA = "Can't find the tag!!!";
          timer_flag = 0;
          break;
        }
      }
      break;
  }
}


int UWB_setupmode()       //AT command
{
  switch (UWB_MODE) {
    case 0:
      for (int b = 0; b < 2; b++)
      { //Repeat twice to stabilize the connection
        delay(50);
        Serial2.write("AT+anchor_tag=0\r\n");     //Set up the Tag 设置标签
        delay(50);
        Serial2.write("AT+interval=5\r\n");        //Set the calculation precision, the larger the response is, the slower it will be
        delay(50);                                     //设置计算精度，越大响应越慢
        Serial2.write("AT+switchdis=1\r\n");        //Began to distance 开始测距
        delay(50);
        if (b == 0) {
          Serial2.write("AT+RST\r\n");            //RESET 复位
        }
      }
      UWB_clear();
      break;
    case 1:
      for (int b = 0; b < 2; b++)
      {
        delay(50);
        Serial2.write("AT+anchor_tag=1,");      //Set the base station 设置基站
        Serial2.print(UWB_B_NUMBER);            //UWB_B_NUMBER is base station ID0~ID3
        Serial2.write("\r\n");
        delay(1);
        delay(50);
        if (b == 0) {
          Serial2.write("AT+RST\r\n");
        }
      }
      UWB_clear();
      break;
  }
}


void UWB_Keyscan() {
  if (M5.BtnA.wasReleased())
  {
    UWB_MODE = 0;
    UWB_setupmode();
    UWB_clear();
    UWB_ui_display();
    UWB_T_UI_NUMBER_2 = 0;
  }
  if (M5.BtnB.wasReleased())
  {
    UWB_MODE = 1;
    if (UWB_B_NUMBER == 4)
    {
      UWB_B_NUMBER = 0;
    }
    UWB_setupmode();
    UWB_clear();
    UWB_ui_display();
    UWB_B_NUMBER++;
  }
  if (M5.BtnC.wasReleased())
  {
    Serial2.write("AT+RST\r\n");
    UWB_setupmode();
    UWB_clear();
    UWB_ui_display();
  }
}

void UWB_Timer()
{
  timer = timerBegin(0, 80, true);              //Timer setting 定时器设置
  timerAttachInterrupt(timer, Timer0_CallBack, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);
}

static void IRAM_ATTR Timer0_CallBack(void)      //Timer function 定时器函数
{
  if (timer_flag == 1)
  {
    timer_data++;
    if (timer_data == 4294967280)
    {
      timer_data = 1;
    }
  }
  else
  {
    timer_data = 0;
  }
}

void setup()
{
  M5.begin();
  Serial2.begin(115200, SERIAL_8N1, 13, 14);
  delay(100);
  UWB_Timer();

  UWB_ui_display();

}

void loop()
{
  M5.update();
  UWB_Keyscan();
  UWB_readString();
  UWB_display();
}
