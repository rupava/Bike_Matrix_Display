#include <WebSocketsServer.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <MD_Parola.h>    //for display library
#include <MD_MAX72xx.h>   //for display library
#include <SPI.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8

#define CLK_PIN   14
#define DATA_PIN  13
#define CS_PIN    12

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
String incomingData;
const byte DNS_PORT = 53;
const String moduleName = "BEC-1";
IPAddress apIP(172, 217, 28, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);
String number_client;
WebSocketsServer webSocket = WebSocketsServer(81);

void handleRoot() {

//https://davidjwatts.com/youtube/esp8266/esp-convertHTM.html use this link to convert the ESP code to string its really easy
String html ="<!DOCTYPE html> <html style=\"text-align: center; background-color: #000000; border-style: solid; border-width: 5px; border-color: #FFFFFF;\"> <head> <title>Bike Matrix</title> <meta name=\"viewport\" content=\"width=device-width, minimumscale=1.0, maximum-scale=1.0, initial-scale=1\" /> </head> <body onload=\"javascript:getSocketData()\"> <h1 style=\"color: #009933;\">Bike Matrix Display</h1> <textarea rows=\"10\" cols=\"40\" id=\"message\"></textarea> <br><br> <input id=\"speedSlider\" oninput=\"sliderUpdate()\" type=\"range\" min=\"1\" max=\"50\" value=\"50\"> <input type=\"checkbox\"/> <br> <button type=\"button\" onclick=\"sendSocketData(0);\">UPDATE</button> <button type=\"button\" onclick=\"addChar('À');\">heart</button> <br> <br> </body> <script> var Socket; var i = 0; Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); function getSocketData() { Socket.onmessage = function(event){ var newData = event.data; if(newData.substring(0,1)==\"%\"){ updateChatBox(newData.substring(1)); } if(newData.substring(0,1)==\"&\"){ updateChatBox(newData.substring(1)); } } } function sliderUpdate() { var x = document.getElementById(\"speedSlider\").value; Socket.send(\"`\"+x); } function addChar(ch){ var TheTextBox = document.getElementById(\"message\"); TheTextBox.value = TheTextBox.value + ch + \" \"; } function hex_to_ascii(str1){ var hex = str1.toString(); var str = ''; for (var n = 0; n < hex.length; n += 2) { str += String.fromCharCode(parseInt(hex.substr(n, 2), 16)); } return str; } function sendSocketData(num){ var mess = document.getElementById('message').value; if(checkNull(mess)==false){ alert(\"Please enter your message.\"); } else{ if(num==0){ Socket.send(\"~\"+mess); } } } function checkNull(dump){ if(dump == null || dump.trim() == ''){return false;} else{return true;} } </script> </html>";
webServer.send(200, "text/html", html);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  P.begin();
  
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
}

void loop(){
  dnsServer.processNextRequest();
  webServer.handleClient();
  webSocket.loop();
  if(Serial.available() > 0){
    String showData = Serial.readString();
    P.print(showData);  
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// listens to incoming data from the websocket
void socketListener(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
    if(type == WStype_TEXT){
      incomingData = "";
      if(payload[0] == '~'){
        for(int i = 0; i < length; i++){incomingData += ((char) payload[i]);}
        incomingData.remove(0,1);
        Serial.println(incomingData);
        P.print(incomingData); 
      if(payload[0] == '`'){
        for(int i = 0; i < length; i++){incomingData += ((char) payload[i]);}
        incomingData.remove(0,1);
        Serial.println(incomingData);
        
      }
  }
}
