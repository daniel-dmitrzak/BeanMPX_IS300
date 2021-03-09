#include <BeanMPX.h>
#include <SPI.h>
#include <mcp2515.h>

#define CANID_EXTENDED 0x80000000
#define CANID_BASE_29BIT 0x11230000
#define CANID_BASE_11BIT 0x500
//#define USE_11BIT_ID // For use with cansniffer, which doesn't support 29 bit IDs. We're loosing information about DST-ID but still retain MSG-ID


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
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
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
    #ifdef USE_11BIT_ID
    if(canTempMsg.can_id & 0xF00 == CANID_BASE_11BIT){
    #else
    if(canTempMsg.can_id & 0x9FFF0000 == CANID_BASE_29BIT){
    #endif
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

    if((beanMsgRx[1] & 0x0F) <= 0x08)
    {
      // Repack bean frame into can frame
      #ifdef USE_11BIT_ID
      canMsgTx.can_id = CANID_BASE_11BIT | beanMsgRx[3]&0xFF;
      #else
      canMsgTx.can_id = CANID_EXTENDED | CANID_BASE_29BIT | (beanMsgRx[2] << 8)&0xFF00 | beanMsgRx[3]&0xFF;
      #endif
      canMsgTx.can_dlc = (beanMsgRx[1] & 0x0F) - 2;
      for(int i = 0; i < canMsgTx.can_dlc; i++){
        canMsgTx.data[i] = beanMsgRx[4 + i];
      }
      CANWaitingForSend = true;
    }
  }
  
  if(CANWaitingForSend)
  {
    if(debugEnabled)
    {
      Serial.print("CAN TX: ");
      Serial.print(canMsgTx.can_id, HEX); Serial.print(" ");
      for(int i = 0; i < canMsgTx.can_dlc; i++)
      {
        Serial.print(canMsgTx.data[i], HEX);
        Serial.print(" ");
      }
      Serial.print("DLC: ");
      Serial.print(canMsgTx.can_dlc, DEC); Serial.print(" ");
      Serial.println("");
    }
    
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
    #ifdef USE_11BIT_ID
    beanMsgTx[0] = 0xFE; // DST ID
    beanMsgTx[1] = (canMsgRx.can_id & 0xFF); // MSG ID 
    #else
    beanMsgTx[0] = (canMsgRx.can_id & 0xFF00) >> 8; // DST ID
    beanMsgTx[1] = (canMsgRx.can_id & 0xFF); // MSG ID 
    #endif
    
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
    
    if (!bean.isBusy()) {
      bean.sendMsg(beanMsgTx, beanTxSize); 
    }
    beanWaitingForSend = false;
  }
}









//    /\_____/\
//   /  o   o  \
//  ( ==  ^  == )
//   )         ( 
//  (           )
// ( (  )   (  ) )
//(__(__)___(__)__)
