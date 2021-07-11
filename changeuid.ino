  #include <SPI.h>
  #include <PN532_SPI.h>
  #include "PN532.h"

  PN532_SPI pn532spi(SPI, 15);
  PN532 nfc(pn532spi);
  int cardplace=0;
    
void setup(void) {
  Serial.begin(115200);
  Serial.println("Hello!");
  pinMode(2, INPUT);  
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  nfc.setPassiveActivationRetries(0xFF);
  
  nfc.SAMConfig();  
  Serial.println("Waiting for an ISO14443A card");
}

void loop(void) {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  
  uint8_t uidLength;    
  
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    if (cardplace==1){
      Serial.println("\nFound a card!");
      //Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
      Serial.print("UID Value: ");
      for (uint8_t i=0; i < uidLength; i++) 
      {
        Serial.print(" 0x");
        Serial.print(uid[i], HEX); 
      }
      Serial.println("");    

       int switchStatus = digitalRead(2);
      uint8_t data1[] = {0xCB,0xA6,0xBC,0x77,0xA6,0x08,0x04,0x00,0x99,0x81,0x85,0x65,0x78,0x82,0x65,0x89};
      uint8_t data2[] = {0x19,0x2A,0x67,0xE4,0xB0,0x08,0x04,0x00,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69};
      if (switchStatus==1){
         writeToBlock0(data1);
       }
       else if(switchStatus==0){
         writeToBlock0(data2);
       }
     
      Serial.println("Finish,please take the card away\n");
      cardplace=2;
    }
  }
  else
  {
    if(cardplace==0){
      Serial.println("Timed out waiting for a card");
      cardplace=1;
    }
    else if (cardplace==2){
      cardplace=0;
      Serial.println("Please wait.");
      }
  }
}





void writeToBlock0(uint8_t data[]){
  int result;
  //uint8_t changeuid[]= { 0xCB, 0xA6, 0xBC, 0x77};
  // FF 00 00 00 08 D4 08 63 02 00 63 03 00
  // Defines the transmission data rate and framing during transmission.
  // Disables CRC,106kbits, 14443A
  result=nfc.writeRegister(0x6302,0);
  if(!result){
    Serial.println("Write 6302 00 Failed.");
    return;
  }
  delay(1000);
  Serial.println("Write 6302 00 Success.");  
  
  // Defines the transmission data rate and framing during reciving.
  // Disables CRC,106kbits, 14443A
  result=nfc.writeRegister(0x6303,0);
  if(!result){
    Serial.println("Write 6303 00 Failed.");
    return;
  }
  delay(1000);
  Serial.println("Write 6303 00 Success.");
  
  /*
   * Send MIFARE Halt command, since CRC is disabled during transmission, CRC of 57 CD is sent in the command
   */
  // FF 00 00 00 06 D4 42 50 00 57 CD
  uint8_t cmd[] = {0x50,0,0x57,0xCD};
  result=nfc.inCommunicateThru(cmd,4);
  if(!result){
    Serial.println("50 00 57 CD Failed.");
    return;
  }
  delay(1000);
  Serial.println("50 00 57 CD Success.");

  // FF 00 00 00 05 D4 08 63 3D 07
  // Adjustments for bit oriented frames
  // 07 means only 7 bits of last bytes is transmitted
  result=nfc.writeRegister(0x633D,7);
  if(!result){
    Serial.println("Write 633D 07 Failed.");
    return;
  }
  delay(1000);
  Serial.println("Write 633D 07 Success.");

  // FF 00 00 00 03 D4 42 40
  // BackDoor Command
  cmd[0] = 0x40;
  result=nfc.inCommunicateThru(cmd,1);
  if(!result){
    Serial.println("40 Failed.");
    return;
  }
  delay(1000);
  Serial.println("40 Success.");

  // FF 00 00 00 05 D4 08 63 3D 00
  // 00 means all 8 bits of last type is transmitted
  result=nfc.writeRegister(0x633D,0);
  if(!result){
    Serial.println("Write 633D 00 Failed.");
    return;
  }
  delay(1000);
  Serial.println("Write 633D 00 Success.");

  // FF 00 00 00 03 D4 42 43
  // BackDoor Command
  cmd[0] = 0x43;
  result=nfc.inCommunicateThru(cmd,1);
  if(!result){
    Serial.println("43 Failed.");
    return;
  }
  delay(1000);
  Serial.println("43 Success.");  

  // FF 00 00 00 08 D4 08 63 02 80 63 03 80
  // Enables CRC,106kbits, 14443A/Mifare Framing for transmission
  result=nfc.writeRegister(0x6302,0x80);
  if(!result){
    Serial.println("Write 6302 80 Failed.");
    return;
  }
  delay(1000);
  Serial.println("Write 6302 80 Success.");  

  // Enables CRC,106kbits, 14443A/Mifare Framing for reception
  result=nfc.writeRegister(0x6303,0x80);
  if(!result){
    Serial.println("Write 6303 80 Failed.");
    return;
  }
  delay(1000);
  Serial.println("Write 6303 80 Success.");

  // Write to Block 0
  //uint8_t data[] = {0xCB,0xA6,0xBC,0x77,0xA6,0x08,0x04,0,0x99,0x81,0x85,0x65,0x78,0x82,0x65,0x89};
  result=nfc.mifareclassic_WriteDataBlock(0,data);
  if(!result){
    Serial.println("Write Failed.");
    return;
  }
  delay(1000);
  Serial.println("Write Success.");
}
