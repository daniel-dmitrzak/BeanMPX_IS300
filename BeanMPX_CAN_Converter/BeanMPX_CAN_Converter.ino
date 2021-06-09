#include <BeanMPX.h>
#include <EMUcan.h>

#define BEAN_PERIOD 100
#define CAN_PERIOD 100

EMUcan emucan = EMUcan(0x600, 10);
BeanMPX bean;

// Bean Messages                    {DID,  MID,  DAT0, DAT1, DAT2}
uint8_t beanMsg_26_Tacho[]        = {0xFE, 0x26, 0x1A, 0xA0, 0xA0}; // Tacho value 
uint8_t beanMsg_2C_EngineTemp[]   = {0x62, 0x2C, 0x67}; // Engine Temp D0 90-255
uint8_t beanMsg_40_Gear[]         = {0xFE, 0x40, 0x00, 0x00}; // A/T transmission gear. We have manual so it's allways be all 0
uint8_t beanMsg_52[]              = {0x40, 0x52, 0xA0, 0xA5}; // Dunno what's this yet. Does it ever change? 
uint8_t beanMsg_A4_FuelCons[]     = {0x62, 0xA4, 0x0A}; // Fuel consumption in L/h. Can be calculated
uint8_t beanMsg_CD_OutsideTemp[]  = {0xFE, 0xCD, 0x38, 0xA8}; // Outside temperature
uint8_t beanMsg_D2_DriveMode[]    = {0x62, 0xD2, 0x12, 0x00, 0xff}; // Drive mode, Cruise control and snow mode 
uint8_t beanMsg_D4_EngineFlags[]  = {0xFE, 0xD4, 0x30}; // Engine flags 
uint8_t beanMsg_F8[]              = {0x98, 0xF8, 0x45}; // Dunno what's that. 
uint8_t beanMsg_24_Speed[]        = {0xFE, 0x24, 0x0A}; // Vehicle speed

// Timings 
uint16_t beanPeriod_26_Tacho      = 500; 
uint16_t beanPeriod_2C_EngineTemp = 1000; 
uint16_t beanPeriod_40_Gear       = 1000;
uint16_t beanPeriod_52            = 5000;
uint16_t beanPeriod_A4_FuelCons   = 0; // Disable this?  
uint16_t beanPeriod_CD_OutsideTemp= 1000; 
uint16_t beanPeriod_D2_DriveMode  = 1000;
uint16_t beanPeriod_D4_EngineFlags= 1000;
uint16_t beanPeriod_F8            = 1000;
uint16_t beanPeriod_24_Speed      = 500;

// Timers 
uint32_t beanTimer_26_Tacho      = 0; 
uint32_t beanTimer_2C_EngineTemp = 0;
uint32_t beanTimer_40_Gear       = 0;
uint32_t beanTimer_52            = 0;
uint32_t beanTimer_A4_FuelCons   = 0; // Disable this?  
uint32_t beanTimer_CD_OutsideTemp= 0; 
uint32_t beanTimer_D2_DriveMode  = 0;
uint32_t beanTimer_D4_EngineFlags= 0;
uint32_t beanTimer_F8            = 0;
uint32_t beanTimer_24_Speed      = 0;

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
  // Drive mode flags
  bool cruiseOn = false; 
  bool snowOn = false; 
  // Engine flags 
  bool starterRunning = false; 
  bool noCharge = false; 
  bool lowOilPress = false;
};

lexusData myLexusData;

// Functions 
void readBean();
void sendBean();
void plotBeanData();
void plotEmuData();

bool enableSniffer = true;

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
  pinMode(9, OUTPUT);
  pinMode(8, INPUT_PULLUP);
  
  beanTimer_26_Tacho = millis() + beanPeriod_26_Tacho;
  beanTimer_2C_EngineTemp = millis() + beanPeriod_2C_EngineTemp + 20;
  beanTimer_40_Gear = millis() + beanPeriod_40_Gear + 60;
  beanTimer_52 = millis() + beanPeriod_52 + 100; 
  beanTimer_A4_FuelCons = millis() + beanPeriod_A4_FuelCons + 130; 
  beanTimer_CD_OutsideTemp = millis() + beanPeriod_CD_OutsideTemp + 190; 
  beanTimer_D2_DriveMode = millis() + beanPeriod_D2_DriveMode + 210;
  beanTimer_D4_EngineFlags = millis() + beanPeriod_D4_EngineFlags + 250;
  beanTimer_F8 = millis() + beanPeriod_F8 + 300;
  beanTimer_24_Speed = millis() + beanPeriod_24_Speed + 320;
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
      Serial.print(millis(),DEC); Serial.print(": ");
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
//      case 0x26: // Tacho 
//        myLexusData.rpm = (beanBuffer[4] << 8 | beanBuffer[5]) / 5.12;
//        break;
      case 0x24: // Speedo
        myLexusData.vehicleSpeed = beanBuffer[4];
        break;
//      case 0x2C: // Engine Temp
//        myLexusData.engineTemp = beanBuffer[4] / 2 ;
//        break;
//      case 0xCD: // Outside Temp 
//        myLexusData.outsideTemp = beanBuffer[4] - 48;
//        break;
      case 0xD7: // Air conditioning flags
        myLexusData.acOn = ((beanBuffer[4] >> 6) & 0x01) ? true : false;
        break;
      case 0xA4: // Fuel level 
        myLexusData.fuelLevel = beanBuffer[4];
        break;
//      case 0xD2:
//        myLexusData.cruiseOn = ((beanBuffer[4] & 0x02) != 0);
//        myLexusData.snowOn = ((beanBuffer[4] & 0x10) != 0);
//        break;
//      case 0xD4:
//        myLexusData.starterRunning = ((beanBuffer[4] & 0x80) != 0);
//        myLexusData.noCharge = ((beanBuffer[4] & 0x20) != 0);
//        myLexusData.lowOilPress = ((beanBuffer[4] & 0x10) != 0);
//        break;
      default:
        break;
    }
  }
}

int beanMessageToSend = 3;
void sendBean()
{  
  int convertedRpm = emucan.emu_data.RPM * 5.12;

  if(beanTimer_26_Tacho <= millis())
  {
//    beanMsg_26_Tacho[2] = convertedRpm & 0xFF00 >> 8;
//    beanMsg_26_Tacho[3] = convertedRpm & 0xFF;
    while(bean.isBusy());
    bean.sendMsg(beanMsg_26_Tacho, sizeof(beanMsg_26_Tacho)); 
    beanTimer_26_Tacho = millis() + beanPeriod_26_Tacho; 
  }
  if(beanTimer_2C_EngineTemp <= millis())
  {
    //beanMsg_2C_EngineTemp[2] = emucan.emu_data.CLT + 48;
    while(bean.isBusy());
    bean.sendMsg(beanMsg_2C_EngineTemp, sizeof(beanMsg_2C_EngineTemp)); 
    beanTimer_2C_EngineTemp = millis() + beanPeriod_2C_EngineTemp;
  }
  if(beanTimer_40_Gear <= millis())
  {
    while(bean.isBusy());
    bean.sendMsg(beanMsg_40_Gear, sizeof(beanMsg_40_Gear));
    beanTimer_40_Gear = millis() + beanPeriod_40_Gear; 
  }
  if(beanTimer_52 <= millis())
  {
    while(bean.isBusy());
    bean.sendMsg(beanMsg_52, sizeof(beanMsg_52));
    beanTimer_52 = millis() + beanPeriod_52;
  }
//  if(beanTimer_A4_FuelCons <= millis())
//  {
//    while(bean.isBusy());
//    bean.sendMsg(beanMsg_A4_FuelCons, sizeof(beanMsg_A4_FuelCons));
//    beanTimer_A4_FuelCons = millis() + beanPeriod_A4_FuelCons;
//  }
  if(beanTimer_CD_OutsideTemp <= millis())
  {
    while(bean.isBusy());
    bean.sendMsg(beanMsg_CD_OutsideTemp, sizeof(beanMsg_CD_OutsideTemp)); 
    beanTimer_CD_OutsideTemp = millis() + beanPeriod_CD_OutsideTemp;
  }
  if(beanTimer_D2_DriveMode <= millis())
  {
    while(bean.isBusy());
    bean.sendMsg(beanMsg_D2_DriveMode, sizeof(beanMsg_D2_DriveMode));
    beanTimer_D2_DriveMode = millis() + beanPeriod_D2_DriveMode;
  }
  if(beanTimer_D4_EngineFlags <= millis())
  {
    while(bean.isBusy());
    bean.sendMsg(beanMsg_D4_EngineFlags, sizeof(beanMsg_D4_EngineFlags));
    beanTimer_D4_EngineFlags = millis() + beanPeriod_D4_EngineFlags;
  }
  if(beanTimer_F8 <= millis())
  {
    while(bean.isBusy());
    bean.sendMsg(beanMsg_F8, sizeof(beanMsg_F8));
    beanTimer_F8 = millis() + beanPeriod_F8;
  }
  if(beanTimer_24_Speed <= millis())
  {
    while(bean.isBusy());
    bean.sendMsg(beanMsg_24_Speed, sizeof(beanMsg_24_Speed));
    beanTimer_24_Speed = millis() + beanPeriod_24_Speed;
  }
//  if (!bean.isBusy()) {
//      if(beanMessageToSend == 1)
//      {
//          beanMsg_26_Tacho[2] = convertedRpm & 0xFF00 >> 8;
//          beanMsg_26_Tacho[3] = convertedRpm & 0xFF;
//          bean.sendMsg(beanMsg_26_Tacho, sizeof(beanMsg_26_Tacho)); 
//      }
//      else if(beanMessageToSend == 2)
//      {
//          beanMsg_2C_EngineTemp[2] = emucan.emu_data.CLT + 48;
//          bean.sendMsg(beanMsg_2C_EngineTemp, sizeof(beanMsg_2C_EngineTemp)); 
//      }
//      else if(beanMessageToSend == 3)
//      {
//        bean.sendMsg(beanMsg_40_Gear, sizeof(beanMsg_40_Gear));
//      }
//      else if(beanMessageToSend == 4)
//      {
//        bean.sendMsg(beanMsg_52, sizeof(beanMsg_52));
//      }
//     else if(beanMessageToSend == 5) 
//     {
//        bean.sendMsg(beanMsg_A4_FuelCons, sizeof(beanMsg_A4_FuelCons));
//     }
//     else if(beanMessageToSend == 6)
//     {
//        bean.sendMsg(beanMsg_CD_OutsideTemp, sizeof(beanMsg_CD_OutsideTemp));
//     }
//     else if(beanMessageToSend == 7)
//     {
//        bean.sendMsg(beanMsg_D2_DriveMode, sizeof(beanMsg_D2_DriveMode));
//     }
//     else if(beanMessageToSend == 8)
//     {
//        bean.sendMsg(beanMsg_D4_EngineFlags, sizeof(beanMsg_D4_EngineFlags));
//     }
//     else if(beanMessageToSend == 9)
//     {
//        bean.sendMsg(beanMsg_F8, sizeof(beanMsg_F8));
//     }
//      else
//        beanMessageToSend = 0;
//      beanMessageToSend++;
//    }
}

void plotBeanData()
{
  Serial.print("rpm:"); Serial.print(myLexusData.rpm, DEC); Serial.print(",");
  Serial.print("vss:"); Serial.print(myLexusData.vehicleSpeed, DEC); Serial.print(",");
  Serial.print("temp:"); Serial.print(myLexusData.engineTemp, DEC); Serial.print(",");
  Serial.print("out:"); Serial.print(myLexusData.outsideTemp, DEC); Serial.print(",");
  Serial.print("fuel:"); Serial.print(myLexusData.fuelLevel, DEC); Serial.print(",");
  // Flags
  Serial.print("ac:"); Serial.print(myLexusData.acOn ? 255 : 0, DEC); Serial.print(",");
  Serial.print("start:"); Serial.print(myLexusData.starterRunning ? 255 : 0, DEC); Serial.print(",");
  Serial.print("nochg:"); Serial.print(myLexusData.noCharge ? 255 : 0, DEC); Serial.print(",");
  Serial.print("lowoil:"); Serial.print(myLexusData.lowOilPress ? 255 : 0, DEC); Serial.print(",");
  Serial.print("cruise:"); Serial.print(myLexusData.cruiseOn ? 255 : 0, DEC); Serial.print(",");
  Serial.print("snow:"); Serial.print(myLexusData.snowOn ? 255 : 0, DEC); Serial.print(",");
  Serial.println("");
}

void plotEmuData()
{
  Serial.print("ecutemp:"); Serial.print(emucan.emu_data.emuTemp, DEC); Serial.print(",");
  Serial.print("iat:"); Serial.print(emucan.emu_data.IAT, DEC); Serial.print(",");
  Serial.print("map:"); Serial.print(emucan.emu_data.MAP, DEC); Serial.print(",");
  Serial.print("batt:"); Serial.print(emucan.emu_data.Batt, DEC); Serial.print(",");
  Serial.println("");
}

void loop() {
  emucan.checkEMUcan();

  readBean();

  //plotBeanData();
  //plotEmuData();

  sendBean();

  if (canTimer < millis()) {
    canMsg1.data[0] = myLexusData.fuelLevel;
    canMsg1.data[1] = myLexusData.vehicleSpeed / 2;
    canMsg1.data[2] = myLexusData.acOn ? 0x255 : 0x0;
    //Sends the frame;
    emucan.sendFrame(&canMsg1);
    canTimer = millis() + CAN_PERIOD;
  }
}
