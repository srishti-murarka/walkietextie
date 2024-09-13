#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Arduino.h>
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif
#include <Keypad.h>

//=========================================KEYPAD=================================================================
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'},
};

byte rowPins[ROWS] = {A8,A9,A10,A11}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {A12,A13,A14}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

char prevkey = NO_KEY;
long long int count =0;
int tap=0;
char mp[9][4]={
  {'a','b','c','1'},
  {'d','e','f','2'},
  {'g','h','i','3'},
  {'j','k','l','4'},
  {'m','n','o','5'},
  {'p','q','r','6'},
  {'s','t','u','7'},
  {'v','w','x','8'},
  {'y','z','0','9'}
  };

char mp1[5]={' ','.',',','?','!'};
//=========================================KEYPAD=================================================================

//=========================================DISPLAY=================================================================
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
String messageToSend = "I: ";
String messageReceived = "";

void updateDisplay(){
  
  u8g2.clearBuffer();
  for (int i=0; i<=messageReceived.length(); i=i+15) {
    u8g2.drawStr(0, (int)((i*2)/3) +10, messageReceived.substring(i,min(i+15,messageReceived.length())).c_str());
  }
  for (int i=0; i<=messageToSend.length(); i=i+15) {
    u8g2.drawStr(0, (int)((i*2)/3) +30, messageToSend.substring(i,min(i+15,messageToSend.length())).c_str());
  }
  u8g2.sendBuffer();
}


//=========================================DISPLAY=================================================================

//=========================================RF_TRANSCEIVER=================================================================
RF24 radio(9, 8); // CE, CSN
const byte srishtiAddr[6] = "00420";
const byte ipsitaAddr[6] = "00069";
bool recFlag = true;

void sendMsg()
{
  Serial.println(F("Sending..."));
  radio.stopListening();
  bool rslt;
  int key = 33 + random(10);
  const char replyData[messageToSend.length()]; 
  char encryptedData[messageToSend.length()+1];
  messageToSend.toCharArray(replyData, messageToSend.length()+1);
  for (int i=0; i<messageToSend.length(); i++){
    if (i&1) encryptedData[i]=char(int(replyData[i])+key-33);
    else encryptedData[i]=char(int(replyData[i])-key+33);
  }
  encryptedData[messageToSend.length()]=char(key);
  Serial.println(replyData);
  Serial.println(encryptedData);
  rslt = radio.write( &encryptedData, sizeof(encryptedData) );
  radio.startListening();

  if (rslt) {
    Serial.println(F("Acknowledge Received"));
    messageToSend = "I: Message Sent";
    updateDisplay();
    delay(2000);
    messageToSend = "I:";
    updateDisplay();
  }
  else {
    Serial.println(F("Tx failed"));
  }
}

void receiveMsg()
{
  char dataReceived[21] = "";
  radio.read( &dataReceived, sizeof(dataReceived) );
  messageReceived = String(dataReceived);
  Serial.println("Encrypted Message: ");
  Serial.println(messageReceived);
    const char encryptedMessage[messageReceived.length()];
    messageReceived.toCharArray(encryptedMessage, messageReceived.length());
    char key = messageReceived[messageReceived.length()-1];
    messageReceived="";
    for (int i=0; i<sizeof(encryptedMessage)-1; i++){
      if (i&1) messageReceived+=char(int(encryptedMessage[i])-int(key)+33);
      else messageReceived+=char(int(encryptedMessage[i])+int(key)-33);
    }
    Serial.println("Message Received: ");
    Serial.println(messageReceived);
    updateDisplay();
    
  if (messageReceived=="") recFlag = false;
}

//=========================================RF_TRANSCEIVER=================================================================

//=========================================SETUP=================================================================

void setup()
{
  //---------SerialSetup---------
  Serial.begin(9600);
  Serial.println("::Ipsita's Window::");
  //---------SerialSetup---------

  //---------RFSetup---------
  radio.begin();
  radio.setDataRate( RF24_250KBPS );
  radio.openWritingPipe(srishtiAddr); // NB these are swapped compared to the master
  radio.openReadingPipe(1, ipsitaAddr);
  radio.setRetries(3,5); // delay, count
  radio.startListening();
  //---------RFSetup---------

  //---------DisplaySetup---------
  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.clearBuffer();
  u8g2.drawStr(0, 30, "I: ");
  u8g2.sendBuffer();
  //---------DisplaySetup---------
}
//=========================================SETUP=================================================================


//=========================================LOOP=================================================================
void loop()
{
   if ( radio.available() && recFlag) receiveMsg();
   recFlag = true;  

  char key = keypad.getKey();
  if (key!=NO_KEY)
    Serial.println(key);

  if(count>40000)
  {
    count =0;
    if(prevkey!=NO_KEY)
    {
      if (messageToSend.length()<=20)
        {
          if (prevkey>= '1' && prevkey <= '9')
          messageToSend += mp[prevkey-'1'][(tap-1)%(4)];
          else if (prevkey == '0')
          messageToSend += mp1[(tap-1)%(5)];
          else if (prevkey == '*'){
            for(int i = 0;i<tap;i++) if(messageToSend.length()>3) messageToSend.remove(messageToSend.length()-1);
          }
        }

      // Update display
      updateDisplay();
    }

    prevkey = NO_KEY;
    tap=1;
  }
  // If a key is pressed
  if (key != NO_KEY){
    if (key == '#') {
      sendMsg();
    }

    count=0;
    // If it's a digit or * or #
    if (key == '1' || key == '2' || key == '3' || key == '4' || key == '5' || key == '6' || key == '7' || key == '8' || key == '9' || key == '0' || key == '*') {
      // Add the key to the messageToSend
        if(key==prevkey)
        {
          tap++;
        }
        else
        {
          tap = 1;
        }
          prevkey =key;
      }
      
    }
  count++;
}
//=========================================LOOP=================================================================
