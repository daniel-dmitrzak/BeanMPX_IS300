#include <BeanMPX.h>

BeanMPX bean;
uint32_t timer = 0;

void setup() {
  bean.ackMsg((const uint8_t[]) {0xFE}); // Messages to acknowledge
  bean.begin();  
  
  Serial.begin(115200);
  Serial.println("BeanMPX");
}

void loop() {  
  if (bean.available()) {
    Serial.print(bean.msgType()); 
    Serial.print(" ");    
    while (bean.available()) {      
      Serial.print(bean.read(), HEX); 
      Serial.print(" ");    
    }
    Serial.print("\n");    
  }
  
}
