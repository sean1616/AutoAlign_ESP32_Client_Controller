#include <WiFi.h>
#include <esp_now.h>
#include <U8g2lib.h>
#include <EEPROM.h>
// #include <EEPROM_Controller.h>
#include <EEPROM_Control.h>

EEPROM_Control EP_C(34);

/* Keyboard Pin Setting */
const byte R_0 = 12;
const byte R_1 = 14;
const byte R_2 = 27;
const byte R_3 = 26;
const byte C_1 = 25;
const byte C_2 = 33;
const byte C_3 = 32;

int delayBetweenStep_X = 8;
int delayBetweenStep_Y = 8;
int delayBetweenStep_Z = 8;

int AQ_Scan_Compensation_Steps_Z_A = 12;

double ref_Dac = 0; //PD reference
double ref_IL = 0;  //PD reference

double Target_IL = 0; //0 dB



bool btn_isTrigger = false;

int cmd_No = 0;

int hallVal=1;
String SSD, Msg;
String contr_Name = "C001::";

uint8_t broadcastAddress[] = {0x8C, 0x4B, 0x14, 0x16, 0x65, 0xFC}; 
// uint8_t broadcastAddress[] = {0x8C, 0x4B, 0x14, 0x16, 0xD8, 0xD0}; 

typedef struct struct_send_message {
    String contr_name;
    char cmd[30];
    char value[20];
    // String value;
} struct_send_message;

// typedef struct struct_message {
//     String msg;
// } struct_message;

typedef struct struct_receive_msg_UI_Data {
    String msg;
    double _Target_IL;
    int _Q_Z_offset;
    double _ref_Dac;
    int _speed_x;
    int _speed_y;
    int _speed_z;
} struct_receive_msg_UI_Data;

// Create a struct_message called BME280Readings to hold sensor readings
struct_send_message sendmsg;

// Create a struct_message to hold incoming sensor readings
// struct_message incomingReadings;

struct_receive_msg_UI_Data incoming_UI_Data;

U8G2_ST7920_128X64_F_SW_SPI lcd(U8G2_R0, 5, 18, 19, U8X8_PIN_NONE); //data 4 , en, rs

int LCD_Encoder_A_pin = 22; //22
int LCD_Encoder_B_pin = 23; //23
uint8_t LCD_Select_pin = 21;  //21

bool LCD_Encoder_State = false;
bool LCD_Encoder_LastState = false;
int LCD_en_count = 0, idx = 0;
int LCD_sub_count = 0, idx_sub = 0;
int current_selection = 0;

unsigned long Q_Time = 0;
unsigned long LCD_Auto_Update_TimeCount = 0;

int LCD_Update_Mode = 0;
uint8_t LCD_PageNow = 1;

bool isStop = false;

bool isLCD = true;
bool isLCD_Auto_Update = false;

#define ITEMS_COUNT 100
char *UI_Items[ITEMS_COUNT] =
    {" "};

#define MENU_ITEMS 6
char *UI_Menu_Items[MENU_ITEMS] =
    {"1. Status",
     "2. Target IL",
     "3. StableDelay",
     "4. Q Z-offset",
     "5. Speed",
     "6. Get Ref"};

#define Speed_Page_ITEMS 4
char *UI_Speed_Page_Items[MENU_ITEMS] =
    {"1. X Speed",
     "2. Y Speed",
     "3. Z Speed",
     "<<"};

uint8_t i, h, w, title_h, H;

int PageLevel = 0;
int PageItemsCount = 1;

int Top_Item_Index = 0;
int Bottom_Item_Index = 3;
bool ui_YesNo_Selection = false;


int mainpage_SelectedBox_Index = 0;
// int mainpageIndex = 0;
int subpage_SelectedBox_Index = 0;
// int subpageIndex = 0;
int subpage_itemsCount = 1;
bool item_is_selected = false;
bool plus_minus = false;
int pre_LCD_Page_index = 0;

// void Task_1_sendData(void *pvParameters)
// {
//   while (true)
//   {
//     if (!digitalRead(LCD_Select_pin))
//     {
//       LCD_Encoder_Selected();
//     }

//     int idx = LCD_en_count / 2;
//     // Serial.println(String(idx));
//     updateUI(idx);

//     if (isLCD_Auto_Update)
//       if (millis() - LCD_Auto_Update_TimeCount > 5000)
//       {
//         LCD_Auto_Update_TimeCount = millis();
//         isLCD = true;
//       }

//     delay(150);
//     // lcd.clearDisplay();
//     // delay(150);

//     //Task1休息，delay(1)不可省略
//     delay(1);
//   }
// }

//Full Page method
void updateUI(int pageIndex)
{
  if (isLCD)
  {
    if (pageIndex > MENU_ITEMS - 1)
      pageIndex = MENU_ITEMS - 1;

    lcd.clearBuffer();

    H = lcd.getHeight();
    h = lcd.getFontAscent() - lcd.getFontDescent() + 2;
    w = lcd.getWidth();
    title_h = h + 2;
    lcd.drawBox(0, h + 1, w, 1); //Seperate Line

    // Serial.println("LCD_PageNow:" + String(LCD_PageNow) + ",Page Specific Index:" + String(pageIndex) + ",mainpage_SelectedBox_Index:" + String(mainpage_SelectedBox_Index));

    if(!item_is_selected)
    {
      // "UI?".toCharArray(sendmsg.cmd, 60);
      DataSent_Core("UI?", "");
      delay(30);
      DataReceive_Core(); //Call ESP-Now receive data function
    }
      
    if (PageLevel == 0) //Main Page (Menu)
    {
      PageItemsCount = MENU_ITEMS;

      int title_w = (w / 2) - (lcd.getStrWidth("Menu") / 2);
      lcd.drawStr(title_w, h - 1, "Menu"); //Draw Title

      int deltaIndex = 0;
      if (pageIndex > Bottom_Item_Index)
      {
        deltaIndex = abs(pageIndex - Bottom_Item_Index);
        Top_Item_Index = Top_Item_Index + deltaIndex;
        Bottom_Item_Index = Top_Item_Index + 3;
      }
      else if (pageIndex < Top_Item_Index)
      {
        deltaIndex = abs(Top_Item_Index - pageIndex);
        Top_Item_Index = Top_Item_Index - deltaIndex;
        Bottom_Item_Index = Top_Item_Index + 3;
      }

      //Draw each item in UI_Menu_Items
      Draw_ALL_UI_Items(LCD_Update_Mode, pageIndex);

      if (pageIndex < PageItemsCount - 1)
        lcd.drawTriangle(w / 2 - 2, H - 3, w / 2 + 3, H - 3, w / 2, H); //Draw Triangle

      mainpage_SelectedBox_Index = pageIndex;

      // lcd.sendBuffer();
    }

    else if (PageLevel == 1)
    {
      if (mainpage_SelectedBox_Index == 4)
      {
        Draw_ALL_UI_Items(LCD_Update_Mode, pageIndex);
      }
      else if (mainpage_SelectedBox_Index == 5)  //Get Ref
      {
        int H = lcd.getHeight();

        int title_w = (w / 2) - (lcd.getStrWidth("Get Ref ?") / 2);
        lcd.drawStr(title_w, H / 2 - (h / 2), "Get Ref ?");

        lcd.drawBox(0, H / 2 - 1.8 * h, w, 1); //Seperate Line
        lcd.drawBox(0, H / 2 + 1.8 * h, w, 1); //Seperate Line

        int location_X_Yes = (w / 2) - 7 - lcd.getStrWidth("Yes");
        int location_X_No = (w / 2) + 7;
        int location_Y = H / 2 + 0.9 * h;

        lcd.drawStr(location_X_Yes, location_Y, "Yes");
        lcd.drawStr(location_X_No, location_Y, "No");

        // Serial.println("YES NO :" + String(ui_YesNo_Selection));
        // Serial.println("PageLevel:" + String(PageLevel) + ", mainpage_SelectedBox_Index:" + String(mainpage_SelectedBox_Index));

        //Draw Selection box
        if (ui_YesNo_Selection)
          lcd.drawFrame(location_X_Yes - 2, location_Y - h + 1, lcd.getStrWidth("Yes") + 4, h + 2);
        else
          lcd.drawFrame(location_X_No - 2, location_Y - h + 1, lcd.getStrWidth("No") + 4, h + 2);

        LCD_PageNow = LCD_Update_Mode;

        // lcd.sendBuffer();
      }
    }
    
    else if (PageLevel == 101 || PageLevel == 102) /* Auto-Aligning */
    {
      lcd.clearBuffer();

      h = lcd.getFontAscent() - lcd.getFontDescent() + 2;
      w = lcd.getWidth();

      int H = lcd.getHeight();

      int title_w = (w / 2) - (lcd.getStrWidth("Auto-Aligning") / 2);
      lcd.drawStr(title_w, H / 2, "Auto-Aligning");

      lcd.drawBox(0, H / 2 - h, w, 1); //Seperate Line
      lcd.drawBox(0, H / 2 + 5, w, 1); //Seperate Line

      lcd.sendBuffer();
    }

    else if (PageLevel == 103) /* Auto-Curing */
    {
      lcd.clearBuffer();

      h = lcd.getFontAscent() - lcd.getFontDescent() + 2;
      w = lcd.getWidth();

      int H = lcd.getHeight();

      int title_w = (w / 2) - (lcd.getStrWidth("Auto-Curing") / 2);
      lcd.drawStr(title_w, H / 2 - (h / 2), "Auto-Curing");

      lcd.drawBox(0, H / 2 - 1.8 * h, w, 1); //Seperate Line
      lcd.drawBox(0, H / 2 + 1.8 * h, w, 1); //Seperate Line

      String Q_Time_Show = String(Q_Time) + " s";
      int Q_Time_w = (w / 2) - (lcd.getStrWidth(Q_Time_Show.c_str()) / 2);
      lcd.drawStr(Q_Time_w, H / 2 + 0.9 * h, Q_Time_Show.c_str());

      lcd.sendBuffer();
    }

    lcd.sendBuffer();
    isLCD = false;

    return;

    if (LCD_Update_Mode == 0 && pageIndex != LCD_PageNow) /* Main Page*/
    {
      // lcd.begin();
      lcd.clearBuffer();
      // lcd.clearDisplay();

      H = lcd.getHeight();
      h = lcd.getFontAscent() - lcd.getFontDescent() + 2;
      w = lcd.getWidth();
      title_h = h + 2;

      int title_w = (w / 2) - (lcd.getStrWidth("Menu") / 2);
      lcd.drawStr(title_w, h - 1, "Menu");

      lcd.drawBox(0, h + 1, w, 1); //Seperate Line

      int deltaIndex = 0;
      if (pageIndex > Bottom_Item_Index)
      {
        deltaIndex = abs(pageIndex - Bottom_Item_Index);
        Top_Item_Index = Top_Item_Index + deltaIndex;
        Bottom_Item_Index = Top_Item_Index + 3;
      }
      else if (pageIndex < Top_Item_Index)
      {
        deltaIndex = abs(Top_Item_Index - pageIndex);
        Top_Item_Index = Top_Item_Index - deltaIndex;
        Bottom_Item_Index = Top_Item_Index + 3;
      }

      // if (LCD_Update_Mode < 100)
      //   lcd.drawFrame(0, (pageIndex - Top_Item_Index) * h + 1 + title_h, w, h + 1); //Un-Selected Box

      //Draw each item in UI_Menu_Items
      Draw_ALL_UI_Items(LCD_Update_Mode, pageIndex);

      if (pageIndex < MENU_ITEMS - 1)
        lcd.drawTriangle(w / 2 - 2, H - 3, w / 2 + 3, H - 3, w / 2, H);

      LCD_PageNow = pageIndex;

      lcd.sendBuffer();
    }

    else if (LCD_Update_Mode == 1) /* Auto-Aligning */
    {
      // lcd.clearBuffer();
      lcd.clearDisplay();
      // lcd.clearWriteError();

      h = lcd.getFontAscent() - lcd.getFontDescent() + 2;
      w = lcd.getWidth();

      int H = lcd.getHeight();

      int title_w = (w / 2) - (lcd.getStrWidth("Auto-Aligning") / 2);
      lcd.drawStr(title_w, H / 2, "Auto-Aligning");

      lcd.drawBox(0, H / 2 - h, w, 1); //Seperate Line
      lcd.drawBox(0, H / 2 + 5, w, 1); //Seperate Line

      lcd.sendBuffer();
    }

    else if (LCD_Update_Mode == 2) /* Auto-Curing */
    {
      lcd.clearBuffer();
      // lcd.clearDisplay();
      // lcd.clearWriteError();

      h = lcd.getFontAscent() - lcd.getFontDescent() + 2;
      w = lcd.getWidth();

      int H = lcd.getHeight();

      int title_w = (w / 2) - (lcd.getStrWidth("Auto-Curing") / 2);
      lcd.drawStr(title_w, H / 2 - (h / 2), "Auto-Curing");

      lcd.drawBox(0, H / 2 - 1.8 * h, w, 1); //Seperate Line
      lcd.drawBox(0, H / 2 + 1.8 * h, w, 1); //Seperate Line

      String Q_Time_Show = String(Q_Time) + " s";
      int Q_Time_w = (w / 2) - (lcd.getStrWidth(Q_Time_Show.c_str()) / 2);
      lcd.drawStr(Q_Time_w, H / 2 + 0.9 * h, Q_Time_Show.c_str());

      lcd.sendBuffer();
    }

    else if (LCD_Update_Mode == 12) /* Target IL */
    {
      lcd.clearBuffer();
      // lcd.clearDisplay();

      int title_w = (w / 2) - (lcd.getStrWidth("Menu") / 2);
      lcd.drawStr(title_w, h - 1, "Menu");

      lcd.drawBox(0, h + 1, w, 1); //Seperate Line

      //Draw each item in UI_Menu_Items
      Draw_ALL_UI_Items(LCD_Update_Mode, pageIndex);

      if (pageIndex < MENU_ITEMS - 1)
        lcd.drawTriangle(w / 2 - 2, H - 3, w / 2 + 3, H - 3, w / 2, H);

      LCD_PageNow = LCD_Update_Mode;

      lcd.sendBuffer();
    }

    else if (LCD_Update_Mode == 14) /* Q Z-offset */
    {
      lcd.clearBuffer();
      // lcd.clearDisplay();

      int title_w = (w / 2) - (lcd.getStrWidth("Menu") / 2);
      lcd.drawStr(title_w, h - 1, "Menu");

      lcd.drawBox(0, h + 1, w, 1); //Seperate Line

      //Draw each item in UI_Menu_Items
      Draw_ALL_UI_Items(LCD_Update_Mode, pageIndex);

      if (pageIndex < MENU_ITEMS - 1)
        lcd.drawTriangle(w / 2 - 2, H - 3, w / 2 + 3, H - 3, w / 2, H);

      LCD_PageNow = LCD_Update_Mode;

      lcd.sendBuffer();
    }

    else if (LCD_Update_Mode == 15) /* X Speed */
    {
      lcd.clearBuffer();
      // lcd.clearDisplay();

      int title_w = (w / 2) - (lcd.getStrWidth("Menu") / 2);
      lcd.drawStr(title_w, h - 1, "Menu");

      lcd.drawBox(0, h + 1, w, 1); //Seperate Line

      //Draw each item in UI_Menu_Items
      Draw_ALL_UI_Items(LCD_Update_Mode, pageIndex);

      if (pageIndex < MENU_ITEMS - 1)
        lcd.drawTriangle(w / 2 - 2, H - 3, w / 2 + 3, H - 3, w / 2, H);

      LCD_PageNow = LCD_Update_Mode;

      lcd.sendBuffer();
    }

    else if (LCD_Update_Mode == 99) /* Get Ref ? */
    {
      lcd.clearBuffer();

      h = lcd.getFontAscent() - lcd.getFontDescent() + 2;
      w = lcd.getWidth();

      int H = lcd.getHeight();

      int title_w = (w / 2) - (lcd.getStrWidth("Get Ref ?") / 2);
      lcd.drawStr(title_w, H / 2 - (h / 2), "Get Ref ?");

      lcd.drawBox(0, H / 2 - 1.8 * h, w, 1); //Seperate Line
      lcd.drawBox(0, H / 2 + 1.8 * h, w, 1); //Seperate Line

      int location_X_Yes = (w / 2) - 7 - lcd.getStrWidth("Yes");
      int location_X_No = (w / 2) + 7;
      int location_Y = H / 2 + 0.9 * h;

      lcd.drawStr(location_X_Yes, location_Y, "Yes");
      lcd.drawStr(location_X_No, location_Y, "No");

      //Draw Selection box
      if (ui_YesNo_Selection)
        lcd.drawFrame(location_X_Yes - 2, location_Y - h + 1, lcd.getStrWidth("Yes") + 4, h + 2);
      else
        lcd.drawFrame(location_X_No - 2, location_Y - h + 1, lcd.getStrWidth("No") + 4, h + 2);

      LCD_PageNow = LCD_Update_Mode;

      lcd.sendBuffer();
    }

    else if (LCD_Update_Mode == 100) /*  */
    {
      // lcd.initDisplay();

      // delay(100);

      // lcd.begin();
      // lcd.clearBuffer();
      lcd.clearDisplay();
      // lcd.clearWriteError();
      delay(200);

      H = lcd.getHeight();
      h = lcd.getFontAscent() - lcd.getFontDescent() + 2;
      w = lcd.getWidth();
      title_h = h + 2;

      // lcd.setCursor(3, h);
      // lcd.print("Menu");
      int title_w = (w / 2) - (lcd.getStrWidth("Menu") / 2);
      lcd.drawStr(title_w, h - 1, "Menu");

      lcd.drawBox(0, h + 1, w, 1); //Seperate Line

      int start_i = 0; //start_i<=2
      if (pageIndex == 4)
        start_i = 1;
      else if (pageIndex == 5)
        start_i = 2;

      lcd.drawFrame(0, (pageIndex - start_i) * h + 1 + title_h, w, h + 1); //Select Box

      //Draw each item in UI_Menu_Items
      for (i = start_i; i < start_i + 4; i++)
      {
        lcd.drawStr(3, title_h + ((i + 1 - start_i) * h) - 1, UI_Menu_Items[i]);

        switch (i)
        {
        case 1:
          lcd.drawStr(lcd.getWidth() - lcd.getStrWidth("-00.0") - 2, title_h + ((i + 1 - start_i) * h) - 1, String(Target_IL).c_str());
          break;

        default:
          break;
        }
      }

      if (pageIndex < MENU_ITEMS - 1)
        lcd.drawTriangle(w / 2 - 2, H - 3, w / 2 + 3, H - 3, w / 2, H);

      LCD_PageNow = pageIndex;

      lcd.sendBuffer();
    }

    isLCD = false;

    // LCD_en_count =
  }
}

void Draw_ALL_UI_Items(int LCD_Update_Mode, int pageIndex)
{
  //Draw each item in UI_Menu_Items
  if (PageLevel == 1 && mainpage_SelectedBox_Index == 4) //Speed mode
  {
    for (i = 0; i < Speed_Page_ITEMS; i++)
    {
      lcd.drawStr(3, title_h + ((i + 1) * h) - 1, UI_Speed_Page_Items[i]);

      //Item Content
      switch (i)
      {
      case 0:
        lcd.drawStr(lcd.getWidth() - lcd.getStrWidth("0000") - 2, title_h + ((i + 1) * h) - 1, String(delayBetweenStep_X).c_str());
        break;

      case 1:
        lcd.drawStr(lcd.getWidth() - lcd.getStrWidth("0000") - 2, title_h + ((i + 1) * h) - 1, String(delayBetweenStep_Y).c_str());
        break;

      case 2:
        lcd.drawStr(lcd.getWidth() - lcd.getStrWidth("0000") - 2, title_h + ((i + 1) * h) - 1, String(delayBetweenStep_Z).c_str());
        break;

      default:
        break;
      }

      if (item_is_selected)
      {
        //Selection Box
        switch (subpage_SelectedBox_Index)
        {
        case 0:
          lcd.drawFrame(lcd.getWidth() - lcd.getStrWidth("0000") - 4, (0) * h + 1 + title_h, lcd.getStrWidth("0000") + 4, h + 1);
          break;

        case 1:
          lcd.drawFrame(lcd.getWidth() - lcd.getStrWidth("0000") - 4, (1) * h + 1 + title_h, lcd.getStrWidth("0000") + 4, h + 1);
          break;

        case 2:
          lcd.drawFrame(lcd.getWidth() - lcd.getStrWidth("0000") - 4, (2) * h + 1 + title_h, lcd.getStrWidth("0000") + 4, h + 1);
          break;

        default:
          break;
        }
      }
      else
      {
        lcd.drawFrame(0, (subpage_SelectedBox_Index)*h + 1 + title_h, w, h + 1); //Un-Selected Box
      }
    }
  }
  else //Main Page (Menu) Items
  {
    for (i = Top_Item_Index; i < Top_Item_Index + 4; i++)
    {
      if (PageLevel == 0)
      {
        for (size_t j = 0; j < PageItemsCount; j++)
        {
          UI_Items[j] = UI_Menu_Items[j];
        }
      }

      lcd.drawStr(3, title_h + ((i + 1 - Top_Item_Index) * h) - 1, UI_Items[i]);

      //Item Content
      switch (i)
      {
      case 1:
        lcd.drawStr(lcd.getWidth() - lcd.getStrWidth("-00.0") - 2, title_h + ((i + 1 - Top_Item_Index) * h) - 1, String(Target_IL).c_str());
        break;

      case 3:
        lcd.drawStr(lcd.getWidth() - lcd.getStrWidth("0000") - 2, title_h + ((i + 1 - Top_Item_Index) * h) - 1, String(AQ_Scan_Compensation_Steps_Z_A).c_str());
        break;

      case 5:
        lcd.drawStr(lcd.getWidth() - lcd.getStrWidth("0000.00") - 2, title_h + ((i + 1 - Top_Item_Index) * h) - 1, String(ref_Dac).c_str());
        break;

      default:
        break;
      }

      if (item_is_selected)
      {
        //Selection Box
        switch (LCD_Update_Mode)
        {
        case 12:
          lcd.drawFrame(lcd.getWidth() - lcd.getStrWidth("-00.0") - 4, (pageIndex - Top_Item_Index) * h + 1 + title_h, lcd.getStrWidth("-00.0") + 4, h + 1);
          break;

        case 14:
          lcd.drawFrame(lcd.getWidth() - lcd.getStrWidth("0000") - 4, (pageIndex - Top_Item_Index) * h + 1 + title_h, lcd.getStrWidth("0000") + 4, h + 1);
          break;

        default:
          break;
        }
      }
      else
      {
        lcd.drawFrame(0, (pageIndex - Top_Item_Index) * h + 1 + title_h, w, h + 1); //Un-Selected Box
      }
    }
  }
}

void LCD_Encoder_Rise()
{
  LCD_Encoder_State = digitalRead(LCD_Encoder_A_pin);

  bool is_update_LCD_en_count = false;

  if (LCD_Encoder_State != LCD_Encoder_LastState)
  {
    if (digitalRead(LCD_Encoder_B_pin) != LCD_Encoder_State)
    {
      if (PageLevel == 0 && item_is_selected)
      {
        if (mainpage_SelectedBox_Index == 1)
        {
          Target_IL += 0.05;
        }
        else if (mainpage_SelectedBox_Index == 3)
        {
          AQ_Scan_Compensation_Steps_Z_A += 1;
        }
      }
      else if (PageLevel == 1)
      {
        if (mainpage_SelectedBox_Index == 4)  //Speed Setting Page
        {
          if (!item_is_selected)
          {
            if (subpage_SelectedBox_Index < (subpage_itemsCount - 1) * 2){
              LCD_sub_count +=1;
              subpage_SelectedBox_Index += 1;
            }
          }
          else
          {
            if (subpage_SelectedBox_Index == 0)
              delayBetweenStep_X += 1;
            else if (subpage_SelectedBox_Index == 1)
              delayBetweenStep_Y += 1;
            else if (subpage_SelectedBox_Index == 2)
              delayBetweenStep_Z += 1;
          }
        }
        else if (mainpage_SelectedBox_Index == 5)
        {
          ui_YesNo_Selection = false;
        }
      }

      if (PageLevel == 0 && !item_is_selected)
      {
        is_update_LCD_en_count = true;
        LCD_en_count++;
      }
    }
    else
    {
      if (PageLevel == 0 && item_is_selected)
      {
        if (mainpage_SelectedBox_Index == 1)
        {
          Target_IL -= 0.05;
        }
        else if (mainpage_SelectedBox_Index == 3)
        {
          AQ_Scan_Compensation_Steps_Z_A -= 1;
        }
      }
      else if (PageLevel == 1)
      {
        if (mainpage_SelectedBox_Index == 4)
        {
          if (!item_is_selected)
          {
            if (subpage_SelectedBox_Index > 0){
              LCD_sub_count-=1;
              subpage_SelectedBox_Index -= 1;
            }
          }
          else
          {
            if (subpage_SelectedBox_Index == 0)
              delayBetweenStep_X -= 1;
            else if (subpage_SelectedBox_Index == 1)
              delayBetweenStep_Y -= 1;
            else if (subpage_SelectedBox_Index == 2)
              delayBetweenStep_Z -= 1;
          }
        }
        else if (mainpage_SelectedBox_Index == 5)
        {
          ui_YesNo_Selection = true;
        }
      }

      if (PageLevel == 0 && !item_is_selected)
      {
        is_update_LCD_en_count = true;
        LCD_en_count--;
      }
    }
  }
  LCD_Encoder_LastState = LCD_Encoder_State;

  idx = LCD_en_count / 2;

  if (PageLevel == 1)
  {
    if (mainpage_SelectedBox_Index == 4)
    {
      if (!item_is_selected)
        subpage_SelectedBox_Index = LCD_sub_count / 2;

        if(subpage_SelectedBox_Index > 3){
          subpage_SelectedBox_Index = 3;
          LCD_sub_count = 6;
        }
    }
  }

  isLCD = true;
}

void LCD_Encoder_Selected()
{
  if (!btn_isTrigger)
  {
    btn_isTrigger = true;

    // Serial.println("PageLevel:" + String(PageLevel) + ", mainpage_SelectedBox_Index:" + String(mainpage_SelectedBox_Index) + ", subpage_SelectedBox_Index:" + String(subpage_SelectedBox_Index));
    // Serial.println("subpage_SelectedBox_Index:" + String(subpage_SelectedBox_Index) + ", item_is_selected:" + String(item_is_selected));
      // Serial.println("LCD_Encoder_Selected");

    //主頁面(第一層)
    if (PageLevel == 0)
    {
      // Serial.println("Mainpage_Index:" + String(mainpage_SelectedBox_Index));

      if (!item_is_selected)
      {
        switch (mainpage_SelectedBox_Index)
        {
        case 1: /* Into Target IL Mode*/
          LCD_Update_Mode = 12;
          item_is_selected = true;
          isLCD = true;
          pre_LCD_Page_index = mainpage_SelectedBox_Index;
          break;
        case 3: /* Into Q Z-offset Mode*/
          LCD_Update_Mode = 14;
          item_is_selected = true;
          isLCD = true;
          pre_LCD_Page_index = mainpage_SelectedBox_Index;
          break;

        case 4: /* Into Speed Mode*/
          LCD_Update_Mode = 101;
          isLCD = true;
          PageLevel = 1;
          subpage_itemsCount = Speed_Page_ITEMS;
          subpage_SelectedBox_Index = 0;
          pre_LCD_Page_index = mainpage_SelectedBox_Index;
          break;

        case 5: /* Into Get Ref Mode*/
          LCD_Update_Mode = 99;
          isLCD = true;
          PageLevel = 1;
          pre_LCD_Page_index = mainpage_SelectedBox_Index;
          break;

        // case 99:
        //   if (ui_YesNo_Selection)
        //     cmd_No = 19; //Get Ref

          LCD_Update_Mode = 0;
          isLCD = true;
          break;

        default:
          break;
        }
      }
      else
      {
        switch (mainpage_SelectedBox_Index)
        {
        case 1: /* Set Target IL */
          LCD_en_count = 2;
          LCD_Update_Mode = 0;
          isLCD = true;
          DataSent_Core("UI_Data_Target_IL", String(Target_IL));  
          item_is_selected = false;
          break;

        case 3: /* Set Q Z-offset */
          LCD_en_count = 6;
          LCD_Update_Mode = 0;
          isLCD = true;
          DataSent_Core("UI_Data_Z_offset", String(AQ_Scan_Compensation_Steps_Z_A));
          item_is_selected = false;
          break;

        default:
          break;
        }
      }
      updateUI(pre_LCD_Page_index);
    }
    
    //第二層頁面
    else if (PageLevel == 1)
    {
      if (mainpage_SelectedBox_Index == 4) // speed mode
      {
        if (subpage_SelectedBox_Index != 3)
        {
          // sendmsg.cmd = "UI_Data";
          // sendmsg.value = "UI_Data";
          // sendmsg_UI_Data._Target_IL = Target_IL;
          // sendmsg_UI_Data._ref_Dac = ref_Dac;
          // sendmsg_UI_Data._Q_Z_offset = AQ_Scan_Compensation_Steps_Z_A;
          // sendmsg_UI_Data._speed_x = delayBetweenStep_X;
          // sendmsg_UI_Data._speed_y = delayBetweenStep_Y;
          // sendmsg_UI_Data._speed_z = delayBetweenStep_Z;
          // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg_UI_Data, sizeof(sendmsg_UI_Data));

          if (item_is_selected)
          {
            switch (subpage_SelectedBox_Index)
            {
            case 0:
              DataSent_Core("UI_Data_speed_x", String(delayBetweenStep_X));
              break;

            case 1:
              DataSent_Core("UI_Data_speed_y", String(delayBetweenStep_Y));
              break;

            case 2:
              DataSent_Core("UI_Data_speed_z", String(delayBetweenStep_Z));
              break;

            default:
              break;
            }
          }

          item_is_selected = !item_is_selected;
        }
        else
        {
          item_is_selected = false;
          PageLevel = 0;
        }
        isLCD = true;
      }

      else if (mainpage_SelectedBox_Index == 5) //get ref mode
      {
        PageLevel = 0;

        if (ui_YesNo_Selection)
        {
          Serial.println("Get Ref!");
          ui_YesNo_Selection = false;
          // cmd_No = 19; //Get Ref
          DataSent_Core("UI_Data_Ref", String(19));  

          // LCD_en_count = 2;
          // LCD_Update_Mode = 0;
          // DataSent_Core("UI_Data_Target_IL", String(Target_IL));
          // item_is_selected = false;

          delay(300);
        }

        isLCD = true;
        // updateUI(5);
      }
    
    }

    btn_isTrigger = false;
    delay(200);
    return;
  }
}



// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // char macStr[18];
  // Serial.print("Packet to: ");
  // snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
  //          mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  // Serial.println(macStr);
  // Serial.print("Send status: ");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? SSD ="Delivery Success" : SSD ="Delivery Fail");
  status == ESP_NOW_SEND_SUCCESS ? SSD ="Delivery Success" : SSD ="Delivery Fail";
  //
  if(status == 0)
  {
    // Serial.println("OK");
    hallVal= hallVal + 1; // 如果通道成功傳輸後才+1,失敗的話就會繼續傳上一筆的資料
    // Serial.println(contr_Name + String(hallVal));
  }
  //
  // Serial.println();
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incoming_UI_Data, incomingData, sizeof(incoming_UI_Data));
  // Serial.print("Bytes received: ");
  // Serial.println(len);
  Msg = incoming_UI_Data.msg;
  Msg.trim();
  Serial.println("Incoming:" + Msg);

  if(Msg == "Core:FS" || Msg == "Core:AA")
  {
    isLCD = true;
    PageLevel = 101;
    updateUI(PageLevel);
  }
  else if(Msg == "Core:AQ")
  {
    isLCD = true;
    PageLevel = 103;
    updateUI(103);
  }
  else if(Msg == "Core:Menu")
  {
    Target_IL = incoming_UI_Data._Target_IL;
    ref_Dac = incoming_UI_Data._ref_Dac;
    AQ_Scan_Compensation_Steps_Z_A = incoming_UI_Data._Q_Z_offset;
    delayBetweenStep_X = incoming_UI_Data._speed_x;
    delayBetweenStep_Y = incoming_UI_Data._speed_y;
    delayBetweenStep_Z = incoming_UI_Data._speed_z;

    isLCD = true;
    PageLevel = 0;
    updateUI(PageLevel);
  }
  else if(Msg == "Core:UI?")
  {
    //  Serial.println("Target_IL:" + String(incoming_UI_Data._Target_IL));
    Target_IL = incoming_UI_Data._Target_IL;
    ref_Dac = incoming_UI_Data._ref_Dac;
    AQ_Scan_Compensation_Steps_Z_A = incoming_UI_Data._Q_Z_offset;
    delayBetweenStep_X = incoming_UI_Data._speed_x;
    delayBetweenStep_Y = incoming_UI_Data._speed_y;
    delayBetweenStep_Z = incoming_UI_Data._speed_z;
  }
  // Serial.println("MSG:" + Msg);
  // Serial.println("Target_IL:" + String(Target_IL));
  // Serial.println("ref_Dac:" + String(ref_Dac));
  // Serial.println("Q_Z_offset:" + String(AQ_Scan_Compensation_Steps_Z_A));
  // Serial.println("delayBetweenStep_X:" + String(delayBetweenStep_X));
  // Serial.println("delayBetweenStep_Y:" + String(delayBetweenStep_Y));
  // Serial.println("delayBetweenStep_Z:" + String(delayBetweenStep_Z));
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50); //設定序列埠接收資料時的最大等待時間

  // led.on();
  EP_C.EE_Begin(512);

   //宣告使用EEPROM 512 個位置
  // EEPROM.begin(512);

  // EEP.EE_Begin(512);
  //  EE_Begin(512);
  // CleanEEPROM(504, 8);

  // WiFi.mode(WIFI_MODE_STA);  // Set device as a Wi-Fi Station
  // Serial.print("ESP32 Board MAC Address:  ");
  // Serial.println(WiFi.macAddress());  //取得本機的MACAddress

  // Init ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  else
    Serial.println("Initializing ESP-NOW");

  // 設置發送數據回傳函數
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // 绑定數據接收端
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6); // Register peer
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  sendmsg.contr_name = contr_Name;

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  // u8g2_font_5x7_tf
  lcd.begin();

  lcd.setFont(u8g2_font_6x10_tf);
  lcd.clearDisplay();

  #pragma region pinMode Setting

  pinMode(R_2, INPUT_PULLUP); // /keyValue:5
  pinMode(R_3, INPUT_PULLUP); // /keyValue:0

  pinMode(C_1, OUTPUT); ///keyValue:1
  pinMode(C_2, OUTPUT); ///keyValue:2
  pinMode(C_3, OUTPUT); ///keyValue:3

  digitalWrite(C_1, false);
  digitalWrite(C_2, false);
  digitalWrite(C_3, false);

  pinMode(R_0, INPUT_PULLUP);
  // attachInterrupt(R_0, EmergencyStop, FALLING);

  pinMode(LCD_Encoder_A_pin, INPUT_PULLUP);                     // /keyValue:0
  pinMode(LCD_Encoder_B_pin, INPUT_PULLUP);                     // /keyValue:0
  pinMode(LCD_Select_pin, INPUT_PULLUP);                        //Encoder switch
  attachInterrupt(LCD_Encoder_A_pin, LCD_Encoder_Rise, CHANGE); //啟用中斷函式(中斷0，函式，模式)
  // attachInterrupt(LCD_Select_pin, LCD_Encoder_Selected, FALLING); //啟用中斷函式(中斷0，函式，模式)

  #pragma endregion

  isLCD = true;
  updateUI(0);
  isLCD = false;
}

void loop() {

  // String rsMsg = "";
  // if (Serial.available())
  //   rsMsg = Serial.readString();
  
  // Send message via ESP-NOW
  // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &mes, sizeof(mes));
  // if(rsMsg!="")
  // {
  //   rsMsg.trim();
  //   // Serial.println(contr_Name + String(rsMsg));
  //   // Set values to send
  //   sendmsg.contr_name = contr_Name;
  //   sendmsg.cmd = rsMsg;
  //   sendmsg.value = "";

  //   Serial.println(sendmsg.contr_name + sendmsg.cmd);

  //   esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
  // }

  // if (result == ESP_OK) {
  //   Serial.println("Sent with success");
  // }
  delay(50);

  #pragma region Keyboard Detect

  int ButtonSelected = KeyValueConverter();
  int cmd_No = Function_Classification("", ButtonSelected);
  cmd_No = Function_Excecutation("", cmd_No);

  #pragma endregion

    // Serial.println("cmd_No: " + String(cmd_No));

    //LCD UI Update
    if (cmd_No < 0)
    {
      // LCD_Encoder_Rise();

      if (!digitalRead(LCD_Select_pin))
      {
        LCD_Encoder_Selected();
        // return;
      }

      //Update UI
      if (true)
      {
        if (idx == pre_LCD_Page_index)
        {
          if (LCD_Update_Mode <= 9)
            isLCD = false;
          else if (LCD_Update_Mode > 9 && isLCD)
            isLCD = true;
          else
            isLCD = false;
        }

        if (!isLCD)
          return;

        if (idx > pre_LCD_Page_index)
        {
          idx = pre_LCD_Page_index + 1;
          pre_LCD_Page_index = idx;
        }
        else if (idx < pre_LCD_Page_index)
        {
          idx = pre_LCD_Page_index - 1;
          pre_LCD_Page_index = idx;
        }
        LCD_en_count = idx * 2;

        if (idx < 0)
        {
          LCD_en_count = 0;
          idx = 0;
        }
        else if (idx >= MENU_ITEMS - 1)
        {
          LCD_en_count = MENU_ITEMS * 2 - 2;
          idx = MENU_ITEMS - 1;
        }

        updateUI(idx);
        isLCD = false;
      }
    }
 
}

//------------------------------------------------------------------------------------------------------------------------------------------

void EmergencyStop()
{
  isStop = true;

  Serial.println("EmergencyStop");

  DataSent_Core("BS", "0");

  // sendmsg.cmd = "BS";
  // sendmsg.value = String(0);
  // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
}

//------------------------------------------------------------------------------------------------------------------------------------------

int KeyValueConverter()
{
  int keyNo = -1;
  bool isKeyPressed = false;
  int keyValueSum = 0;

  if (!digitalRead(R_1))
  {
    isKeyPressed = true;
    keyValueSum += 10;
  }
  else if (!digitalRead(R_2))
  {
    isKeyPressed = true;
    keyValueSum += 5;
  }
  else if (!digitalRead(R_3))
  {
    isKeyPressed = true;
    keyValueSum += 0;
  }

  if (isKeyPressed)
  {
    pinMode(C_1, INPUT_PULLUP);
    pinMode(C_2, INPUT_PULLUP);
    pinMode(C_3, INPUT_PULLUP);

    pinMode(R_1, OUTPUT);
    pinMode(R_2, OUTPUT);
    pinMode(R_3, OUTPUT);

    delay(2);

    if (!digitalRead(C_1))
    {
      keyValueSum += 1;
    }
    else if (!digitalRead(C_2))
      keyValueSum += 2;
    else if (!digitalRead(C_3))
      keyValueSum += 3;
    else
      keyValueSum = 0;

    pinMode(R_1, INPUT_PULLUP);
    pinMode(R_2, INPUT_PULLUP);
    pinMode(R_3, INPUT_PULLUP);

    pinMode(C_1, OUTPUT);
    pinMode(C_2, OUTPUT);
    pinMode(C_3, OUTPUT);

    delay(2);
  }

  if (keyValueSum != 0)
  {
    switch (keyValueSum)
    {
    case 1:
      keyNo = 101; /* Z- */
      break;
    case 2:
      keyNo = 102; /* X+ */
      break;
    case 3:
      keyNo = 103; /* Z+ */
      break;
    case 6:
      keyNo = 104; /* Y+ */
      break;
    case 7:
      keyNo = 105; /* X- */
      break;
    case 8:
      keyNo = 106; /* Y- */
      break;
    case 11:
      keyNo = 7;
      break;
    case 12:
      keyNo = 8;
      break;
    case 13:
      keyNo = 9;
      break;
    default:
      keyNo = -1;
      break;
    }

    isKeyPressed = false;
  }

  if(!digitalRead(R_0))
    keyNo = 0;

  // Serial.println("Keyno:" + String(keyNo));

  return keyNo;
}

//------------------------------------------------------------------------------------------------------------------------------------------

int Function_Classification(String cmd, int ButtonSelected)
{
  if (cmd != "" && ButtonSelected < 0)
  {
    cmd.trim();
    MSGOutput("get_cmd:" + String(cmd));

    String cmdUpper = cmd;
    cmdUpper.toUpperCase();
  }

  int cmd_No = -1;

  //Keyboard - Motor Control
  if (ButtonSelected >= 0)
  {
    
    //Keyboard No. to Cmd Set No.
    cmd_No = ButtonSelected;
    // switch (ButtonSelected)
    // {
    // case 7:
    //   cmd_No = 1;
    //   break;

    // case 8:
    //   cmd_No = 2;
    //   break;

    // case 9:
    //   cmd_No = 3;
    //   break;

    // default:
    //   cmd_No = ButtonSelected;
    //   break;
    // }
  }

  return cmd_No;
}

//------------------------------------------------------------------------------------------------------------------------------------------

int Function_Excecutation(String cmd, int cmd_No)
{
  //Function Execution
  // String cmd = "";
  
   
  if(cmd_No >= 0)
  {
    if (cmd_No != 0)
    {
      // Serial.println("Btn:" + String(ButtonSelected) + ", CMD:" + String(cmd_No));

      //Functions: Alignment
      if (cmd_No <= 100)
      {        
        DataSent_Core("BS", String(cmd_No));
          
        cmd_No = 0;
      }


      //Functions: Motion
      if (cmd_No > 100)
        switch (cmd_No)
        {
          // Function: Cont-------------------------------------------------------------------------
          //Z feed - cont
        case 101:
          while (true)
          {
            DataSent_Core("BS", String(cmd_No));
            // sendmsg.cmd = "BS";
            // sendmsg.value = String(cmd_No);
            // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
            // Serial.println(sendmsg.contr_name + sendmsg.cmd + sendmsg.value);

            if (digitalRead(R_3))
            {
              DataSent_Core("SS", "");
              // sendmsg.cmd = "SS";
              // sendmsg.value = "";
              // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
              Serial.println(sendmsg.contr_name + sendmsg.cmd + sendmsg.value);
              break;
            }    
            delay(50);
          }
          cmd_No = 0;
          break;
        case 103:
          while (true)
          {
            DataSent_Core("BS", String(cmd_No));
            // sendmsg.cmd = "BS";
            // sendmsg.value = String(cmd_No);
            // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
            if (digitalRead(R_3))
            {
              DataSent_Core("SS", "");
              // sendmsg.cmd = "SS";
              // sendmsg.value = "";

              Serial.println(sendmsg.contr_name + sendmsg.cmd + ":" + sendmsg.value);

              // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
              break;
            }    
            delay(50);  //Delay時間太短會導倒core收不到停止指令
          }
          cmd_No = 0;
          break;

          //X feed - cont
        case 102:
          while (true)
          {
            DataSent_Core("BS", String(cmd_No));
            // sendmsg.cmd = "BS";
            // sendmsg.value = String(cmd_No);
            // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
          if (digitalRead(R_3))
          {
            DataSent_Core("SS", "");
              // sendmsg.cmd = "SS";
              // sendmsg.value = "";

              Serial.println(sendmsg.contr_name + sendmsg.cmd + ":" + sendmsg.value);

              // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
              break;
            }    
            delay(50);  //Delay時間太短會導倒core收不到停止指令
          }
          cmd_No = 0;
          break;
          //X+ - cont
        case 105:
          while (true)
          {
            DataSent_Core("BS", String(cmd_No));
            // sendmsg.cmd = "BS";
            // sendmsg.value = String(cmd_No);
            // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
            if (digitalRead(R_2))
            {
              DataSent_Core("SS", "");
            // sendmsg.cmd = "SS";
            //   sendmsg.value = "";

              Serial.println(sendmsg.contr_name + sendmsg.cmd + ":" + sendmsg.value);

              // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
              break;
            }    
            delay(50);  //Delay時間太短會導倒core收不到停止指令
                
          }
          cmd_No = 0;
          break;

          //Y- feed - cont
        case 106:
          while (true)
          {
            DataSent_Core("BS", String(cmd_No));
            // sendmsg.cmd = "BS";
            // sendmsg.value = String(cmd_No);
            // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
            if (digitalRead(R_2))
            {
              DataSent_Core("SS", "");
              // sendmsg.cmd = "SS";
              // sendmsg.value = "";

              Serial.println(sendmsg.contr_name + sendmsg.cmd + ":" + sendmsg.value);

              // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
              break;
            }    
            delay(50);  //Delay時間太短會導倒core收不到停止指令
              
          }
          cmd_No = 0;
          break;

        //Y+ feed - cont
        case 104:
          while (true)
          {
            DataSent_Core("BS", String(cmd_No));
            // sendmsg.cmd = "BS";
            // sendmsg.value = String(cmd_No);
            // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
            if (digitalRead(R_2))
            {
              DataSent_Core("SS", "");
              // sendmsg.cmd = "SS";
              // sendmsg.value = "";

              Serial.println(sendmsg.contr_name + sendmsg.cmd + ":" + sendmsg.value);

              // esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
              break;
            }    
            delay(50);  //Delay時間太短會導倒core收不到停止指令
          }
          cmd_No = 0;
          break;

        //EmergencyStop
        // case 129:
        //   isStop = true;
        //   Serial.println("EmergencyStop");
        //   digitalWrite(Tablet_PD_mode_Trigger_Pin, true); //false is PD mode, true is Servo mode
        //   isLCD = true;
        //   PageLevel = 0;
        //   break;
      
        }
    }
    else
    {
      // Serial.println("cmd_No:" + String(cmd_No));
      EmergencyStop();
    }
  }

  return cmd_No;
}

void DataSent_Core(String CMD, String VALUE)
{
  CMD.toCharArray(sendmsg.cmd, 30);
  VALUE.toCharArray(sendmsg.value, 20);
  // sendmsg.value = VALUE;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &sendmsg, sizeof(sendmsg));
  // Serial.println(CMD + ", " + VALUE);
}

void DataReceive_Core()
{
    //Call ESP-Now receive data function
    esp_now_register_recv_cb(OnDataRecv);
}

//------------------------------------------------------------------------------------------------------------------------------------------

void MSGOutput(String msg)
{
  Serial.println(msg);
}