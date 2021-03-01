#include <BeanMPX.h>

BeanMPX bean;
uint32_t timer = 0;

// Messages           {DID,  MID,  DAT0, DAT1, DAT2}
uint8_t fuel[] =      {0x62, 0xA4, 0x3C}; // Fuel D0 153-40 
uint8_t engTemp[]  =  {0x62, 0x2C, 0xA5}; // Engine Temp D0 90-255 
uint8_t gear[] =      {0x62, 0x40, 0x08, 0x10}; // P,R,N,D,M,3,2,L D0 11111111 (128, 64, 32, 16, 8, 4, 2, 1); Manual mode: 5,4,3,2,L D1 ---11111 (16, 8, 4, 2, 1)
uint8_t ect[] =       {0x62, 0xD2, 0x08, 0x10}; // ECT PWR, SNOW, CRUISE D0 --11--1- (32,16,2); CRUISE-FLASH, ECT PWR-FLASH D1 -11----- (64,32);
uint8_t seatBelt[] =  {0x62, 0xDF, 0x10, 0x80}; // DOOR D0 ---1---- (16);  SEAT BELT D1 1------- (128)
uint8_t tacho[] = {0xFE, 0x26, 0x1A, 0x4C};
// DOOR-FLASH
uint8_t batt[] = {0x62, 0xD4, 0x28}; // DOOR-FLASH, BATT, OIL D0 -111---- (64, 32, 16) 
uint8_t door[] = {0x62, 0xFA, 0xff}; // DOOR-FLASH D0 11111111 (255); *important mid:0xD4 D0:0x28 D0 -1------ must be set first

uint8_t message1[] = {0xFE, 0xAB, 0x40, 0x80}; // Dunno what's that 
uint8_t message2[] = {0xFE, 0x24, 0x00}; // That's speed, we don't need to send that 
uint8_t message3[] = {0xFE, 0xCD, 0x38}; // AC ?
uint8_t message4[] = {0x40, 0x52, 0x98, 0x04}; // Dunno 
uint8_t message5[] = {0x13, 0xD6, 0x36}; // Dunno 
uint8_t message6[] = {0x40, 0xAF, 0x00}; // Dunno 
uint8_t message7[] = {0x98, 0xF8, 0xC0}; // Dunno
uint8_t message8[] = {0xFE, 0x1E, 0x00}; // Dunno 
uint8_t message9[] = {0xFE, 0x2C, 0x9F}; // Engine Temp 
uint8_t message10[] = {0xFE, 0xD4, 0x10}; // More flags, critical oil pressure flag
uint8_t message11[] = {0xFE, 0x26, 0x1A, 0x8C}; // Tacho 
uint8_t message12[] = {0xFE, 0x40, 0x00, 0x00}; // Gear 
uint8_t message13[] = {0xFE, 0xCC, 0x01, 0xED}; // Dunno 
uint8_t message14[] = {0xFE, 0xE4, 0x01, 0x00}; // Dunno 
uint8_t message15[] = {0x62, 0xD2, 0x12, 0x00, 0x00}; // Engine Flags: Cruise control and snow mode
uint8_t message16[] = {0x62, 0x20, 0x09, 0x66}; // Something important i think 
uint8_t message17[] = {0x62, 0xA4, 0x6E, 0x00}; // Dunno 
uint8_t message18[] = {0xFE, 0x90, 0x01, 0x07, 0x20};
int frame = 7;
int delayFrame = 100;
void setup() {
  bean.ackMsg((const uint8_t[]) {0xFE, 0xFF, 0x62}); // Messages to acknowledge
  bean.begin();  
  
  Serial.begin(115200);
  Serial.println("BeanMPX");
  tone(2, 90);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
}
String messageFromSerial; 
bool sendingEnabled = true;

void handleSerialCommand()
{
  while(Serial.available())
  {
    messageFromSerial = Serial.readStringUntil(' ');
    if(messageFromSerial == "HENLO\n")
      Serial.println("Why hello?");
    else if(messageFromSerial.startsWith("TONE"))
    {
      int pin = Serial.readStringUntil(' ').toInt(); 
      int value = Serial.readStringUntil('\n').toInt();
      tone(pin, value);
      Serial.print("Setting tone of pin ");
      Serial.print(pin, DEC);
      Serial.print(" to value ");
      Serial.println( value);
    }
    else if(messageFromSerial.startsWith("SENDING"))
    {
      String setting = Serial.readStringUntil('\n');
      if(setting == "ON")
      {
        sendingEnabled = true;
        Serial.println("Enabling bus simulation");
      }
      else if(setting == "OFF")
      {
        sendingEnabled = false;
        Serial.println("Disabling bus simulation");
      }
    } 
    else if(messageFromSerial.startsWith("DELAY"))
    {
      String setting = Serial.readStringUntil('\n');
      Serial.println(setting);
      //delayFrame = setting;
      //Serial.print("Setting delay to "); Serial.println(delayFrame, DEC);
    }
  }
}

void loop() {  
  
  handleSerialCommand();
  
  if (bean.available()) {
    Serial.print(bean.msgType()); 
    Serial.print(" ");    
    while (bean.available()) {      
      Serial.print(bean.read(), HEX); 
      Serial.print(" ");    
    }
    Serial.print("\n");    
  }
  
   if (timer < millis() && sendingEnabled) {
    if (!bean.isBusy()) {
      switch(frame)
      {
        case 0:
          bean.sendMsg(message3, sizeof(message3)); break;
        case 1:
          bean.sendMsg(message4, sizeof(message4)); break; 
        case 2:
          bean.sendMsg(message5, sizeof(message5)); break;
        case 3: 
          bean.sendMsg(message6, sizeof(message6)); break;
        case 4: 
          bean.sendMsg(message7, sizeof(message7)); break;
        case 5: 
          bean.sendMsg(message8, sizeof(message8)); break; 
        case 6: 
          bean.sendMsg(message9, sizeof(message9)); break;
        case 7:
          bean.sendMsg(message10, sizeof(message10)); break;
        case 8:
          bean.sendMsg(message12, sizeof(message12)); break;
        case 9:
          bean.sendMsg(message13, sizeof(message13)); break;
        case 10:
          bean.sendMsg(message14, sizeof(message14)); break;
        case 11:
          bean.sendMsg(message15, sizeof(message15)); break;
        case 12:
          bean.sendMsg(message16, sizeof(message16)); break; 
        case 13: 
          bean.sendMsg(message17, sizeof(message17)); break; 
        case 15:
          bean.sendMsg(message18, sizeof(message18)); break; 
        default:
          frame = 0;
      }
      timer = millis() + delayFrame;
      //frame++;
    }    
  }
  
}
