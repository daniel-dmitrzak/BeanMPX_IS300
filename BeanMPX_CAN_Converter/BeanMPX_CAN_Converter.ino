#include <BeanMPX.h>

BeanMPX bean;
uint32_t timer = 0;

// Messages           {DID,  MID,  DAT0, DAT1, DAT2}
uint8_t fuel[] =      {0x62, 0xA4, 0x3C}; // Fuel D0 153-40 
uint8_t engTemp[]  =  {0x62, 0x2C, 0x62}; // Engine Temp D0 90-255 
uint8_t gear[] =      {0x62, 0x40, 0x08, 0x10}; // P,R,N,D,M,3,2,L D0 11111111 (128, 64, 32, 16, 8, 4, 2, 1); Manual mode: 5,4,3,2,L D1 ---11111 (16, 8, 4, 2, 1)
uint8_t ect[] =       {0x62, 0xD2, 0x08, 0x10}; // ECT PWR, SNOW, CRUISE D0 --11--1- (32,16,2); CRUISE-FLASH, ECT PWR-FLASH D1 -11----- (64,32);
uint8_t seatBelt[] =  {0x62, 0xDF, 0x10, 0x80}; // DOOR D0 ---1---- (16);  SEAT BELT D1 1------- (128)

uint8_t speedo[] = {0xFE, 0x24, 0x0, 0x67};
uint8_t tacho[] = {0xFE, 0x26, 0x1A, 0x4C, 0xC2};
uint8_t outsideTemp[] = {0xFE, 0xCD, 0x38};
uint8_t acFlags[] = { 0xFE, 0xD7, 0x80};

// DOOR-FLASH
uint8_t batt[] = {0x62, 0xD4, 0x28}; // DOOR-FLASH, BATT, OIL D0 -111---- (64, 32, 16) 
uint8_t door[] = {0x62, 0xFA, 0xff}; // DOOR-FLASH D0 11111111 (255); *important mid:0xD4 D0:0x28 D0 -1------ must be set first

void setup() {
  bean.begin();  
  
  Serial.begin(115200);
  Serial.println("BeanMPX");
  pinMode(10, INPUT_PULLUP);
  tone(2, 150);
}
uint8_t vehicleSpeed = 0;
uint8_t beanBuffer[24] = {0x01, 0x58,  0xFE, 0x2C, 0x62};   
uint8_t ptr = 0; 
void loop() {
  
  if (bean.available()) {
    ptr = 0;
    while (bean.available()) {
      beanBuffer[ptr] = bean.read();
      ptr++; 
    }  
    beanBuffer[ptr] == 0x00; 
    switch( beanBuffer[3])
    {
      case 0x26: // Tacho / 5.12 
        Serial.print("Tacho: ");
        Serial.print((beanBuffer[4] << 8 | beanBuffer[5]) / 5.12, DEC);
        break;
      case 0x24: // Speedo
        Serial.print("Speed: ");
        Serial.print(beanBuffer[4], DEC);
        break; 
      case 0x2C: // Engine Temp 
        Serial.print("Engine Temp: ");
        Serial.print(beanBuffer[4] / 2, DEC);
        break; 
      case 0xCD: // Outside Temp 
        Serial.print("Outside Temp: ");
        Serial.print((beanBuffer[4] - 48), DEC);
        break;
      case 0xD6:
        Serial.print("D6: ");
        Serial.print(beanBuffer[4], DEC);
        break;
      case 0xD7:
        Serial.print("AC: ");
        Serial.print(((beanBuffer[4]>>6)&0x01) ? "ON" : "OFF");
        break;
      case 0xA4:
        Serial.print("Fuel: ");
        Serial.print(beanBuffer[4], DEC);
      default:
        Serial.print("Unknown: ");
        Serial.print(beanBuffer[3], HEX);
        Serial.print(" ");
        Serial.print(beanBuffer[4], HEX);
        break;
    }
    Serial.println("");
  }


   if (timer < millis()) {
    if (!bean.isBusy() && !digitalRead(10)) {      
      //bean.sendMsg(engTemp, sizeof(engTemp));
      //bean.sendMsg(outsideTemp, sizeof(outsideTemp));
      //bean.sendMsg(speedo, sizeof(speedo));
      pinMode(9, OUTPUT);
      bean.sendMsg(tacho, sizeof(tacho));
      pinMode(9, INPUT);
      //bean.sendMsg(acFlags, sizeof(acFlags));
      timer = millis() + 1000;
    } 
   } 
  
}
