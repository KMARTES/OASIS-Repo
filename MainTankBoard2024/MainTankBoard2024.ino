#include "Wire.h"
#include "SDCard.h"
#include "LCD.h"

#include<SoftwareSerial.h>

SoftwareSerial e5(26, 25); // (RX, TX)

////////////////////////////////

#define SDCard_CS 5

///// I2C with ESP32s Variables /////
byte micro_dosing_address = 0x28;
String microDosingData = "NULL";

byte water_leveling_address = 0x76;
String waterLevelingData = "NULL";


bool collectData = false;
/////////////////////////////////////

///// button variables /////
const int amountOfSensors = 5;
String sensorsTitle[amountOfSensors] = {"pH:             ", "TDS:            ", "Water Temp:     ", "Ambient Temp:   ", "Waterflow:      "}; 
String sensorsData[amountOfSensors] = {"NULL", "NULL", "NULL", "NULL", "NULL"}; 

bool updateLCD = true;
bool scrollRight = false;
bool scrollLeft = false;
bool reconfigure = false;

int i = 0;
 
const int left_scroll_button = 35;
const int right_scroll_button = 34;
const int reconfigure_button = 32;
////////////////////////////////////

////// timer variables //////
hw_timer_t * Collect_Timer = NULL;
/////////////////////////////

///// timer interrupts /////
void IRAM_ATTR Collect_Timer_ISR() {
  collectData = true;
}
/////////////////

///// button interrupts /////
void IRAM_ATTR scroll_left() {
  scrollLeft = true;
  updateLCD = true;
}

void IRAM_ATTR scroll_right() {
  scrollRight = true;
  updateLCD = true;
}

void IRAM_ATTR reconfigure_ISR() {
  reconfigure = true;
  updateLCD = true;
}

void setup()
{
  Serial.begin(115200);
  delay(100);

  e5.begin(9600);
  delay(1000);
  e5.write("AT+MODE=TEST\n");
  delay(1000);
  e5.write("AT+TEST=RXLRPKT\n");

   // Begin SDCard //
 if(!SD.begin(SDCard_CS)){
   Serial.println("Card Mount Failed");
   return;
 }
 uint8_t cardType = SD.cardType();

 if(cardType == CARD_NONE){
   Serial.println("No SD card attached");
   return;
 }

 Serial.print("SD Card Type: ");
 if(cardType == CARD_MMC){
   Serial.println("MMC");
 } else if(cardType == CARD_SD){
   Serial.println("SDSC");
 } else if(cardType == CARD_SDHC){
   Serial.println("SDHC");
 } else {
   Serial.println("UNKNOWN");
 }

 uint64_t cardSize = SD.cardSize() / (1024 * 1024);
 Serial.printf("SD Card Size: %lluMB\n", cardSize);

 writeFile(SD, "/theData.txt", "This is data collected today.");
 ///////////////////////////////////////////

  //// setup led ////
  pinMode(led_enable, OUTPUT);
  digitalWrite(led_enable, HIGH);
  Wire.begin(SDA_I2C, SCL_I2C);
  delay(1000);

  lcdInit();
  ///////////////////

  //// setup buttons ////
  pinMode(left_scroll_button, INPUT);
  attachInterrupt(left_scroll_button, scroll_left, FALLING);

  pinMode(right_scroll_button, INPUT);
  attachInterrupt(right_scroll_button, scroll_right, FALLING);

  pinMode(reconfigure_button, INPUT);
  attachInterrupt(reconfigure_button, reconfigure_ISR, FALLING);
  ///////////////////////

  //// set timers ////

    // Set timer frequency to 1Mhz
  Collect_Timer = timerBegin(1000000);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(Collect_Timer, &Collect_Timer_ISR);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter).
  timerAlarm(Collect_Timer, 10000000, true, 0);
  // Collect_Timer = timerBegin(0, 80, true);
  // timerAttachInterrupt(Collect_Timer, Collect_Timer_ISR, true);
  // timerAlarmWrite(Collect_Timer, 10000000, true);
  // timerAlarmEnable(Collect_Timer);
  /////////////////////

  delay(1000);
}

void loop(){

  if(collectData){

    /* collect data from micro-dosing module */
    Serial.println("Collecting Data");
    Wire.requestFrom(micro_dosing_address, 32);
    
    microDosingData = "";
    while(Wire.available()){
      char c = Wire.read();
      microDosingData += c;
    }
    //////////////////////////////////////////////

    /* collect data from water-leveling module */
    Wire.requestFrom(water_leveling_address, 48);
    
    waterLevelingData = "";
    while(Wire.available()){
      char c = Wire.read();
      waterLevelingData += c;
    }
    ///////////////////////////////////////////////

    updateData();

    updateLCD = true;
    collectData = false;
  }
  if (updateLCD and !reconfigure){
    delay(100);
    if (scrollLeft){
      if (i == 0){
        i = amountOfSensors - 1;
      }
      else i--;
      scrollLeft = false;
    }
    else if (scrollRight){
      if (i == amountOfSensors - 1){
        i = 0;
      }
      else i++;
      scrollRight = false;
    }
    lcdClear();
    lcdSetCursor(0, 0);
    lcdWrite(sensorsTitle[i].c_str());

    lcdSetCursor(0, 1);
    lcdWrite(sensorsData[i].c_str());

    updateLCD = false;
  }
  else if(updateLCD and reconfigure){
    delay(100);
    lcdClear();
    lcdSetCursor(0, 0);
    lcdWrite("Reconfiguring...");

    lcdSetCursor(0, 1);
    lcdWrite("       ;)       ");

    updateLCD = false;
    reconfigure = false;
  }
  
  while (e5.available() > 0) {
      Serial.write(e5.read());
  }
}

void updateData(){

  String pHValue = (microDosingData.substring(0, 16));
  String tdsValue = (microDosingData.substring(16, 32));
  String ambientTempValue = (waterLevelingData.substring(0, 16));
  String waterTempValue = (waterLevelingData.substring(16, 32));
  String waterflowValue = (waterLevelingData.substring(32, 48));
  
  sensorsData[0] = pHValue;
  sensorsData[1] = tdsValue;
  sensorsData[2] = ambientTempValue;
  sensorsData[3] = waterTempValue;
  sensorsData[4] = waterflowValue;

  appendFile(SD, "/theData.txt", "ph: ");
  appendFile(SD, "/theData.txt", pHValue.c_str());
  appendFile(SD, "/theData.txt", "\n");

  appendFile(SD, "/theData.txt", "TDS: ");
  appendFile(SD, "/theData.txt", tdsValue.c_str());
  appendFile(SD, "/theData.txt", "\n");

  appendFile(SD, "/theData.txt", "Ambient Temperature: ");
  appendFile(SD, "/theData.txt", ambientTempValue.c_str());
  appendFile(SD, "/theData.txt", "\n");

  appendFile(SD, "/theData.txt", "Water Temperature: ");
  appendFile(SD, "/theData.txt", waterTempValue.c_str());
  appendFile(SD, "/theData.txt", "\n");

  appendFile(SD, "/theData.txt", "Waterflow: ");
  appendFile(SD, "/theData.txt", waterflowValue.c_str());
  appendFile(SD, "/theData.txt", "\n");
}