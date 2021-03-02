#include <BeanMPX.h>
#include <EMUcan.h>

#define BEAN_PERIOD 100
#define CAN_PERIOD 100

EMUcan emucan = EMUcan(0x600, 10);
BeanMPX bean;

// Bean Messages           {DID,  MID,  DAT0, DAT1, DAT2}
uint8_t beanMsg_EngineTemp[]  =  {0x62, 0x2C, 0x62}; // Engine Temp D0 90-255
uint8_t beanMsg_Tacho[] = {0xFE, 0x26, 0x1A, 0x4C, 0xC2};

// CAN stuff
struct can_frame canMsg1;

// Timers 
uint32_t beanTimer = 0;
uint32_t canTimer = 0;

// Struct for holding BEAN data
struct lexusData {
  uint8_t vehicleSpeed = 0;
  uint8_t fuelLevel = 0;
  float rpm = 0;
  float engineTemp = 0;
  float outsideTemp = 0;
  bool acOn = false;
};
lexusData myLexusData;

// Functions 
void readBean();
void sendBean();
void plotBeanData();
void plotEmuData();

bool enableSniffer = false;

void setup() {
  //Setup Bean
  bean.begin();

  //Setup CAN
  emucan.begin(CAN_500KBPS, MCP_8MHZ);
  // Frame to be send:
  canMsg1.can_id  = 0x500;
  canMsg1.can_dlc = 8;
  canMsg1.data[0] = 0xFF;
  canMsg1.data[1] = 0xFF;
  canMsg1.data[2] = 0xFF;

  Serial.begin(115200);
  Serial.println("BeanMPX");
  tone(2, 150);
}

uint8_t beanBuffer[24];
void readBean() {
  uint8_t ptr = 0;

  if (bean.available()) {    
    while (bean.available()) {
      beanBuffer[ptr] = bean.read();
      ptr++;
    }
    
    if(enableSniffer)
    {
      Serial.print(bean.msgType()); 
      Serial.print(" ");    
      for(int i = 0; i < ptr; i++)
      {
        Serial.print(beanBuffer[i], HEX); 
        Serial.print(" "); 
      }
      Serial.print("\n");
    }
    
    switch ( beanBuffer[3] )
    {
      case 0x26: // Tacho 
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
      case 0xD7: // Air conditioning flags
        myLexusData.acOn = ((beanBuffer[4] >> 6) & 0x01) ? true : false;
        break;
      case 0xA4: // Fuel level 
        myLexusData.fuelLevel = beanBuffer[4];
      default:
        break;
    }
  }
}

int beanMessageToSend = 1;
void sendBean()
{
  int convertedRpm = emucan.emu_data.RPM * 5.12;
  if (!bean.isBusy()) {
      if(beanMessageToSend == 1)
      {
          beanMsg_Tacho[2] = convertedRpm & 0xFF00 >> 8;
          beanMsg_Tacho[3] = convertedRpm & 0xFF;
          bean.sendMsg(beanMsg_Tacho, sizeof(beanMsg_Tacho)); 
      }
      else if(beanMessageToSend == 2)
      {
          beanMsg_EngineTemp[2] = emucan.emu_data.CLT + 48;
          bean.sendMsg(beanMsg_EngineTemp, sizeof(beanMsg_EngineTemp)); 
      }
      else
        beanMessageToSend = 0;
      beanMessageToSend++;
    }
}

void plotBeanData()
{
  Serial.print("rpm:"); Serial.print(myLexusData.rpm, DEC); Serial.print(",");
  Serial.print("vss:"); Serial.print(myLexusData.vehicleSpeed, DEC); Serial.print(",");
  Serial.print("temp:"); Serial.print(myLexusData.engineTemp, DEC); Serial.print(",");
  Serial.print("out:"); Serial.print(myLexusData.outsideTemp, DEC); Serial.print(",");
  Serial.print("fuel:"); Serial.print(myLexusData.fuelLevel, DEC); Serial.print(",");
  Serial.print("ac:"); Serial.print(myLexusData.acOn ? 255 : 0, DEC); Serial.print(",");
  Serial.println("");
}

void plotEmuData()
{
  // Empty for now
}

void loop() {
  emucan.checkEMUcan();

  readBean();

  plotBeanData();

  if (beanTimer < millis()){
    sendBean();
    beanTimer = millis() + BEAN_PERIOD;
  }

  if (canTimer < millis()) {
    canMsg1.data[0] = myLexusData.fuelLevel;
    canMsg1.data[1] = myLexusData.vehicleSpeed / 2;
    canMsg1.data[2] = myLexusData.acOn ? 0x255 : 0x0;
    //Sends the frame;
    emucan.sendFrame(&canMsg1);
    canTimer = millis() + CAN_PERIOD;
  }
}
