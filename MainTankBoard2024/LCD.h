#include <Wire.h>

///// lcd variables /////
byte address = 0x3C;
int SDA_I2C = 21;
int SCL_I2C = 22;
int led_enable = 15;
/////////////////////////

void lcdWriteInst(byte inst){
  Wire.beginTransmission(address);
  Wire.write(0x00);
  Wire.write(inst);
  byte error = Wire.endTransmission();
  Serial.println(error);
}

void lcdWriteCharacter(char c){
  Wire.beginTransmission(address);
  Wire.write(0x40);
  Wire.write(c);
  byte error = Wire.endTransmission();
  Serial.println(error);
}

void lcdWrite(String s){
  for(char c : s){
    lcdWriteCharacter(c);
  }
}

void lcdSetCursor(int col, int row){
  if(row == 1){
    lcdWriteInst(0xC0);
  }
  else if(row == 0){
    lcdWriteInst(0x00);
  }
  delay(10);
}

void lcdInit(){
  lcdWriteInst(0x38); // 8 its; 2 lines; 5x8 dots
  delay(10);
  lcdWriteInst(0x0C); // display on; cursor off; blinking off
  delay(10);
  lcdWriteInst(0x01); // clear display
  delay(10);
  lcdWriteInst(0x06); // entry mode set; no increment; no shift
  delay(10);
}

void lcdClear(){
  lcdWriteInst(0x01);
  delay(10);
}
