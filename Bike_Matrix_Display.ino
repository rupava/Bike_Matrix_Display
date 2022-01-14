#include <WebSocketsServer.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#include <MD_Parola.h>    //for display library
#include <MD_MAX72xx.h>   //for display library
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8

#define CLK_PIN   D5
#define CS_PIN    D6
#define DATA_PIN  D7

///////////////////////////////////////////////////////////////////////////////////////////////////////
int scrollSpeed = 50;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 0; // in milliseconds
////////////////////////////////////////////////////////////////////////////////////////////////////////
#define  BUF_SIZE  500

char curMessage[BUF_SIZE] = { "" };
char newMessage[BUF_SIZE] = { "Hello! Enter new message?" };
bool newMessageAvailable = true;



///////////////////////////////////////////////////////////////////////////////////////////////////////

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
String incomingData;
const byte DNS_PORT = 53;

IPAddress apIP(172, 217, 28, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);
String number_client;
WebSocketsServer webSocket = WebSocketsServer(81);

void handleRoot() {
String html ="<!DOCTYPE html> <html style=\"text-align: center; background-color: #000000; border-style: solid; border-width: 5px; border-color: #FFFFFF;\"> <head> <title>Bike Matrix Display</title> <meta name=\"viewport\" content=\"width=device-width, minimumscale=1.0, maximum-scale=1.0, initial-scale=1\" /> </head> <body> <h1 style=\"color: #009933;\">Bike Matrix Display</h1> <textarea rows=\"10\" cols=\"40\" id=\"message\"></textarea> <br> <br> <input id=\"speedSlider\" oninput=\"sliderUpdate();\" type=\"range\" min=\"1\" max=\"100\" value=\"50\"> <br> <button type=\"button\" onclick=\"sendSocketData();\">SEND</button> <br> <br> </body> <script> var Socket; var i = 0; Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); function sendSocketData(){ var mess = document.getElementById('message').value; if(checkNull(mess)==false){ alert(\"Please enter your message.\"); } else{ Socket.send(\"~\"+mess.trim()); } } function checkNull(dump){ if(dump == null || dump.trim() == ''){ return false; } else{ return true; } } function sliderUpdate() { var x = document.getElementById(\"speedSlider\").value; Socket.send(\"`\"+x); } </script> </html>";
webServer.send(200, "text/html", html);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
String eepRead(){
  String readAdd,data,slide;
  
  for (int k = 50; k < 50 + 3; ++k){readAdd += char(EEPROM.read(k));}
  int addInt = readAdd.toInt();
  for (int k = 50 + 3; k < 50 + 6; ++k){slide += char(EEPROM.read(k));}
  int slideS = slide.toInt();
  P.setSpeed(slideS);
  for (int l = 56; l < 56 + addInt; ++l){data += char(EEPROM.read(l));}
  return data;
  }
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  WiFi.mode(WIFI_AP);
  
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Bike Matrix Display","abcd@1234"); // for SSID and setting AP password
  dnsServer.start(DNS_PORT, "*", apIP);
  webServer.onNotFound([]() {
     handleRoot();
  });
  
  webServer.on("/",handleRoot);
  webServer.begin();
  webSocket.begin();
  webSocket.onEvent(socketListener);

  P.begin();
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);

  String stored = eepRead();
  int str_len_stored = stored.length() + 1; 
  char char_array_stored[str_len_stored];
  stored.toCharArray(char_array_stored, str_len_stored);
  strcpy(newMessage,char_array_stored);
  newMessageAvailable = true;
  
}

void loop(){
  dnsServer.processNextRequest();
  webServer.handleClient();
  webSocket.loop();
    if (P.displayAnimate())
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset();
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// listens to incoming data from the websocket

void socketListener(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
    if(type == WStype_TEXT){
      incomingData = "";
     for(int i = 0; i < length; i++){incomingData += ((char) payload[i]);}
     Serial.println(incomingData);
      
     if(payload[0] == '~'){
        incomingData.remove(0,1 );
        eepWrite(incomingData,(int16_t)P.getSpeed());
        int str_len = incomingData.length() + 1; 
        char char_array[str_len];
        incomingData.toCharArray(char_array, str_len);
        strcpy(newMessage,char_array);
        newMessageAvailable = true;
      }
     if(payload[0]=='`'){
        incomingData.remove(0,1);
        P.setSpeed(incomingData.toInt());
      }
    }
}

void eepWrite(String dump, int slideSpeed){
  int len = dump.length();
  String slsped;
  
  
  if(slideSpeed>1 and slideSpeed<=9)  {slsped = "00"+String(slideSpeed);}
  if(slideSpeed>9 and slideSpeed<=99){slsped = "0"+String(slideSpeed);}
  if(slideSpeed==100){slsped = String(slideSpeed);}

  
  if(len>0 and len<=9)  {dump = "00"+String(len)+slsped+dump;}
  if(len>9 and len<=99){dump = "0"+String(len)+slsped+dump;}
  if(len>99 and len<300){dump = String(len)+slsped+dump;}

  for (int i = 0; i < dump.length(); ++i)
  {
    EEPROM.write(50 + i, dump[i]);
  }

  if (EEPROM.commit()) {
//    Serial.println("Data successfully committed");
  } else {
//    Serial.println("ERROR! Data commit failed");
  }
}
