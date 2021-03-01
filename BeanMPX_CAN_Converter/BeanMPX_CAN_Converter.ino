#include <BeanMPX.h>
#include <EMUcan.h>

EMUcan emucan;
BeanMPX bean;

uint32_t timer = 0;

// Messages           {DID,  MID,  DAT0, DAT1, DAT2}
uint8_t fuel[] =      {0x62, 0xA4, 0x3C}; // Fuel D0 153-40
uint8_t gear[] =      {0x62, 0x40, 0x08, 0x10}; // P,R,N,D,M,3,2,L D0 11111111 (128, 64, 32, 16, 8, 4, 2, 1); Manual mode: 5,4,3,2,L D1 ---11111 (16, 8, 4, 2, 1)
uint8_t ect[] =       {0x62, 0xD2, 0x08, 0x10}; // ECT PWR, SNOW, CRUISE D0 --11--1- (32,16,2); CRUISE-FLASH, ECT PWR-FLASH D1 -11----- (64,32);
uint8_t seatBelt[] =  {0x62, 0xDF, 0x10, 0x80}; // DOOR D0 ---1---- (16);  SEAT BELT D1 1------- (128)

uint8_t speedo[] = {0xFE, 0x24, 0x0, 0x67};
uint8_t outsideTemp[] = {0xFE, 0xCD, 0x38};
uint8_t acFlags[] = { 0xFE, 0xD7, 0x80};


uint8_t beanMsg_EngineTemp[]  =  {0x62, 0x2C, 0x62}; // Engine Temp D0 90-255
uint8_t beanMsg_Tacho[] = {0xFE, 0x26, 0x1A, 0x4C, 0xC2};

// CAN stuff
struct can_frame canMsg1;
unsigned long previousMillis = 0;
const long interval = 100;
byte countUp = 0;

void setup() {
  //Setup Bean
  bean.begin();

  //Setup CAN
  emucan.begin(CAN_500KBPS, MCP_8MHZ);
  // Frame to be send:
  canMsg1.can_id  = 0x500;
  canMsg1.can_dlc = 8;
  canMsg1.data[0] = 0x00;
  canMsg1.data[1] = 0x00;
  canMsg1.data[2] = 0x00;

  Serial.begin(115200);
  Serial.println("BeanMPX");
  pinMode(10, INPUT_PULLUP);
  tone(2, 150);

}

struct lexusData {
  uint8_t vehicleSpeed = 0;
  uint8_t fuelLevel = 0;
  float rpm = 0;
  float engineTemp = 0;
  float outsideTemp = 0;
  bool acOn = false;
} ;


lexusData myLexusData;
uint8_t beanBuffer[24] = {0x01, 0x58,  0xFE, 0x2C, 0x62};


void readBean();
void sendBean();
void plotDataOnSerial();
bool enableSniffer = false;

void readBean() {
  uint8_t ptr = 0;

  if (bean.available()) {
    ptr = 0;
    if(enableSniffer)
    {
      Serial.print(bean.msgType()); 
      Serial.print(" ");    
    }
    while (bean.available()) {
      beanBuffer[ptr] = bean.read();
      if(enableSniffer)
      {
        Serial.print(beanBuffer[ptr], HEX); 
        Serial.print(" "); 
      }
      
      ptr++;
    }
    if(enableSniffer) Serial.print("\n");
    beanBuffer[ptr] == 0x00;
    
    switch ( beanBuffer[3] )
    {
      case 0x26: // Tacho / 5.12
        myLexusData.rpm = (beanBuffer[4] << 8 | beanBuffer[5]) / 5.12;
        break;
      case 0x24: // Speedo
        myLexusData.vehicleSpeed = beanBuffer[4];
        break;
      case 0x2C: // Engine Temp
        myLexusData.engineTemp = beanBuffer[4] / 2 ;
        break;
      case 0xCD: // Outside Temp
        myLexusData.outsideTemp = beanBuffer[4] - 48;
        break;
      case 0xD7:
        myLexusData.acOn = ((beanBuffer[4] >> 6) & 0x01) ? true : false;
        break;
      case 0xA4:
        myLexusData.fuelLevel = beanBuffer[4];
      default:
        break;
    }
  }
}
uint8_t beanMessageToSend = 1;

void sendBean()
{
  if (!bean.isBusy()) {
      switch(beanMessageToSend)
      {
        case 1:
          int convertedRpm = emucan.emu_data.RPM * 5.12; 
          beanMsg_Tacho[2] = convertedRpm & 0xFF00 >> 8;
          beanMsg_Tacho[3] = convertedRpm & 0xFF;
          bean.sendMsg(beanMsg_Tacho, sizeof(beanMsg_Tacho)); break;
        case 2:
          beanMsg_EngineTemp[2] = emucan.emu_data.CLT + 48;
          bean.sendMsg(beanMsg_EngineTemp, sizeof(beanMsg_EngineTemp)); break;
        default:
          beanMessageToSend = 0;
          break;
      }
      beanMessageToSend++;
    }
}

void plotDataOnSerial()
{
  Serial.print("rpm:"); Serial.print(myLexusData.rpm, DEC); Serial.print(",");
  Serial.print("vss:"); Serial.print(myLexusData.vehicleSpeed, DEC); Serial.print(",");
  Serial.print("temp:"); Serial.print(myLexusData.engineTemp, DEC); Serial.print(",");
  Serial.print("out:"); Serial.print(myLexusData.outsideTemp, DEC); Serial.print(",");
  Serial.print("fuel:"); Serial.print(myLexusData.fuelLevel, DEC); Serial.print(",");
  Serial.print("ac:"); Serial.print(myLexusData.acOn ? 255 : 0, DEC); Serial.print(",");
  Serial.println("");
}

void loop() {
  emucan.checkEMUcan();

  readBean();

  plotDataOnSerial();
  
//  if (timer < millis()){
//    sendBean();
//    timer = millis() + 1000;
//  }

  // only send every second:
//  unsigned long currentMillis = millis();
//  if (currentMillis - previousMillis >= interval) {
//    previousMillis = currentMillis;
//
//    canMsg1.data[0] = myLexusData.fuelLevel;
//    canMsg1.data[1] = myLexusData.vehicleSpeed;
//    canMsg1.data[2] = myLexusData.acOn ? 0x255 : 0x0;
//
//    //Sends the frame;
//    emucan.sendFrame(&canMsg1);
//  }
}
