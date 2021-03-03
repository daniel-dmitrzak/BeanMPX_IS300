#include <BeanMPX.h>
#include <SPI.h>
#include <mcp2515.h>

#define CANID_FILTER 0x18200000


MCP2515 mcp2515(10);
BeanMPX bean;

// Frame buffers
struct can_frame canMsgRx;
struct can_frame canMsgTx;
uint8_t beanMsgRx[16];
uint8_t beanMsgTx[16];

// Functions 
int readBean();
int readCAN();

// Flags
bool beanWaitingForSend = false;
bool CANWaitingForSend = false; 
bool debugEnabled = true; 

void setup() {
  //Setup Bean
  bean.begin();

  //Setup CAN
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS);
  mcp2515.setNormalMode();

  Serial.begin(115200);
  Serial.println("BeanMPX");
}

int readBean() {
  uint8_t ptr = 0;

  if (bean.available()) {    
    while (bean.available()) {
      beanMsgRx[ptr] = bean.read();
      ptr++;
    }
  }
  
 return ptr;
}

int readCAN()
{
  struct can_frame canTempMsg;
  if (mcp2515.readMessage(&canTempMsg) == MCP2515::ERROR_OK) {   
    if(canTempMsg.can_id & 0xFFF00000 == CANID_FILTER){
      canMsgRx = canTempMsg;
      return canMsgRx.can_dlc;
    }
  }
  return 0;
}

int beanRxSize;
int beanTxSize;
void loop() {
  beanRxSize = readBean();
  if(beanRxSize > 0)
  {
    if(debugEnabled)
    {
      Serial.print("BEAN RX: ");
      for(int i = 0; i < beanRxSize; i++)
      {
        Serial.print(beanMsgRx[i], HEX); Serial.print(" ");
      }
      Serial.println("");
    }

    if(beanMsgRx[1] & 0x0F <= 8)
    {
      // Repack bean frame into can frame
      canMsgTx.can_id = CANID_FILTER | beanMsgRx[2] << 8 | beanMsgRx[3];
      canMsgTx.can_dlc = beanMsgRx[1] & 0x0F;
      for(int i = 0; i < canMsgTx.can_dlc; i++)
        canMsgTx.data[i] = beanMsgRx[4 + i];
      CANWaitingForSend = true;
    }
  }

  if(CANWaitingForSend)
  {
    if(debugEnabled)
    {
      Serial.print("CAN TX: ");
      Serial.print(canMsgTx.can_id, HEX); Serial.print(" ");
      Serial.print(canMsgTx.can_dlc, DEC); Serial.print(" ");
      for(int i = 0; i < canMsgTx.can_dlc; i++)
      {
        Serial.print(canMsgTx.data[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
    }
    //sendCan();
    mcp2515.sendMessage(&canMsgTx);
    CANWaitingForSend = false;
  }
  
  if(readCAN())
  {
    if(debugEnabled)
    {
      Serial.print("CAN RX: ");
      Serial.print(canMsgRx.can_id, HEX); Serial.print(" ");
      Serial.print(canMsgRx.can_dlc, DEC); Serial.print(" ");
      for(int i = 0; i < canMsgRx.can_dlc; i++)
      {
        Serial.print(canMsgRx.data[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
    }
    // Repack can frame into bean frame 
    beanMsgTx[0] = (canMsgRx.can_id & 0xFF00) >> 8; // DST ID
    beanMsgTx[1] = (canMsgRx.can_id & 0xFF); // MSG ID 
    for(int i = 0; i < canMsgRx.can_dlc; i++)
    {
      beanMsgTx[2 + i] = canMsgRx.data[i]; // Data 0
    }
    beanTxSize = canMsgRx.can_dlc + 2; 
    beanWaitingForSend = true; 
  }

  if(beanWaitingForSend)
  {
    if(debugEnabled)
    {
      Serial.print("BEAN TX: ");
      for(int i = 0; i < ((beanMsgTx[1] & 0x0F) + 3); i++)
      {
        Serial.print(beanMsgTx[i], HEX); Serial.print(" ");
      }
      Serial.println("");
    }
    //sendBean();
    if (!bean.isBusy()) {
      bean.sendMsg(beanMsgTx, beanTxSize); 
    }
    beanWaitingForSend = false;
  }
}
