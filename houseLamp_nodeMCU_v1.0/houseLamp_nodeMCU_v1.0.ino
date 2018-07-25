
/*
  >> houseLamp v0.1 <<

  Description: Arduino sketch controlling a LED Strip in a domestic environment. It acts as a web server in a LAN offering
  the user to control the LED Lamp remotely.

  Platform: NodeMCU
  
  Author: Kike Ramírez
  Date: 1/4/2018

  Web: www.kikeramirez.org
  Mail: info@kikeramirez.org
  
  License: MIT

 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>
#include <SPI.h>
#include <FastLED.h>
#include <EEPROM.h>
#include <SimplexNoise.h>


#define LED_RED     LED_BUILTIN

#define USE_SERIAL Serial

#define SSID_HOME "CarmenLauraKike"
#define PWD_HOME "Carmen2016"

// LAMP SETTINGS

// Frames per seconds and step for lerp animations
float fps = 1000.0 / 50.0;
float timeStamp = millis();

float saveStamp = millis();
float saveTimer = 10000.0;

float deltaAnim = 0.04;
// bool needsUpdate = true;


CHSV colorSet = CHSV(0, 0, 0);
CHSV colorLED = CHSV(0, 0, 0);
CHSV colorTarget = CHSV(0, 0, 255);
float colorRatio = 0.0;
bool waveEffect = false;


// Number of LEDS in the strip
#define NUM_LEDS 10

// Arduino Data pin that led data will be written out over
#define DATA_PIN 1

// Object representing the whole strip.
CRGB leds[NUM_LEDS];

// NETWORK SETTINGS

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// String containing html file
String html ="<!DOCTYPE html> <html> <head> <meta charset=\"utf-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"> <link rel=\"apple-touch-icon\" href=\"https://www.kikeramirez.org/images/logo_kikeramirez_16.png\" /> <link rel=\"apple-touch-icon-precomposed\" href=\"https://www.kikeramirez.org/images/logo_kikeramirez_16.png\" /> <link rel=\"shortcut icon\" href=\"https://www.kikeramirez.org/images/logo_kikeramirez_16.png\" /> <link rel=\"icon\" href=\"https://www.kikeramirez.org/images/logo_kikeramirez_16.png\"> <title>LED</title> <meta name=\"description\" content=\"LED Control Site.\"> <meta name=\"keywords\" content=\"creative coding, interaction, arduino, bootstrap, LED, javascript\"> <link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\" type=\"text/css\"> <link rel=\"stylesheet\" href=\"https://www.kikeramirez.org/assets/houseLamp/styleguide.html\"> <link rel=\"stylesheet\" href=\"https://www.kikeramirez.org/assets/houseLamp/theme.css\"> </head> <body class=\"bg-primary\"> <div class=\"py-5 text-center\" style=\"background-size: cover; background-image: url(&quot;http://www.2sega.com/wp-content/uploads/2016/04/LED-lights.jpg&quot;);\"> <div class=\"container py-5\"> <div class=\"row\"> <div class=\"col-md-12\"> <h1 class=\"display-3 mb-4 text-primary\">LED</h1> <p class=\"lead mb-5\"> LED Strip controlled via LAN using a Arduino-based web server.</p> <p class=\"text-secondary mb-5\"> K.Ramírez - April, 2018 - MIT License</p> <canvas id=\"ledColor\" class=\"col-12\" height=\"5%\"></canvas> <button type=\"button\" class=\"col-12 btn btn-lg btn-primary mx-1\" onclick =\"sendON();\">ON</button> <button type=\"button\" class=\"col-12 btn btn-lg btn-primary mx-1\" onclick =\"sendOFF();\">OFF</button> <button type=\"button\" class=\"col-12 btn btn-lg btn-primary mx-1\" onclick =\"effectON();\">EFFECT ON</button> <button type=\"button\" class=\"col-12 btn btn-lg btn-primary mx-1\" onclick =\"effectOFF();\">EFFECT OFF</button> <input id=\"h\" name=\"Hue\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" value=\"0\" oninput=\"sendHue();\" class=\"col-9 btn btn-lg mx-1 btn-secondary\" /> <input id=\"s\" name=\"Saturation\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" value=\"0\" oninput=\"sendSaturation();\" class=\"col-9 btn btn-lg mx-1 btn-secondary\" /> <input id=\"v\" name=\"Brightness\" type=\"range\" min=\"5\" max=\"255\" step=\"1\" value=\"255\" oninput=\"sendBrightness();\" class=\"col-9 btn btn-lg mx-1 btn-secondary\" /> </div> </div> </div> </div> <script src=\"https://code.jquery.com/jquery-3.2.1.slim.min.js\" integrity=\"sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN\" crossorigin=\"anonymous\"></script> <script src=\"https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js\" integrity=\"sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q\" crossorigin=\"anonymous\"></script> <script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js\" integrity=\"sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl\" crossorigin=\"anonymous\"></script> <script> var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']); var hueRange = document.getElementById(\"h\"); var saturationRange = document.getElementById(\"s\"); var brightnessRange = document.getElementById(\"v\"); var c=document.getElementById('ledColor'); var ctx=c.getContext('2d'); connection.onopen = function () { connection.send('Connect ' + new Date()); }; connection.onerror = function (error) { console.log('WebSocket Error ', error); }; connection.onmessage = function (e) { console.log('Server: ', e.data); if(e.data.charAt(0)=='h') hueRange.value = parseInt(e.data.substring(1), 16); if(e.data.charAt(0)=='s') saturationRange.value = parseInt(e.data.substring(1), 16); if(e.data.charAt(0)=='b') brightnessRange.value = parseInt(e.data.substring(1), 16); updateCanvas(); }; function sendHue() { var hue = 'h' + parseInt(document.getElementById('h').value).toString(16); console.log('Hue: ' + hue); connection.send(hue); updateCanvas(); }; function sendSaturation() { var sat = 's' + parseInt(document.getElementById('s').value).toString(16); console.log('Saturation: ' + sat); connection.send(sat); updateCanvas(); }; function sendBrightness() { var bright = \"b\" + parseInt(document.getElementById('v').value).toString(16); console.log('Brightness: ' + bright); connection.send(bright); updateCanvas(); }; function sendON() { var bright = \"bff\"; console.log('ON'); connection.send(bright); brightnessRange.value = 255; }; function sendOFF() { var bright = \"b0\"; console.log('OFF'); connection.send(bright); brightnessRange.value = 0; }; function effectON() { var effectString = \"w1\"; console.log('EFFECT ON'); connection.send(effectString); }; function effectOFF() { var effectString = \"w0\"; console.log('EFFECT OFF'); connection.send(effectString); }; function hsv_to_hsl(h, s, v) { var l = (2 - s) * v / 2; if (l != 0) { if (l == 1) { s = 0; } else if (l < 0.5) { s = s * v / (l * 2); } else { s = s * v / (2 - l * 2); } } return [h, s, l]; }; function updateCanvas() { var hsl = hsv_to_hsl(hueRange.value / 255, saturationRange.value / 255.0, brightnessRange.value / 255.0); ctx.fillStyle='hsl(' + String(hsl[0] * 360.0) + ', ' + String(hsl[1] * 100.0) + '%, ' + String(hsl[2] * 100.0) + '%)'; ctx.fillRect(0,0,c.width,c.height); }; </script> </body> </html>";

SimplexNoise sn;


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

            // send message to client
            webSocket.sendTXT(num, "Connected");

            webSocket.sendTXT(num, "h" + String(colorTarget.h, HEX));
            webSocket.sendTXT(num, "s" + String(colorTarget.s, HEX));
            webSocket.sendTXT(num, "b" + String(colorTarget.v, HEX));

            String dataSent;
            
            if (waveEffect) dataSent = "w1";
            else dataSent = "w0";
            webSocket.sendTXT(num, dataSent);
            
        }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

            if(payload[0] == 'h') {

                // decode rgb data
                colorTarget.h = (uint32_t) strtol((const char *) &payload[1], NULL, 16);
                colorSet = colorLED;
                colorRatio = 0.0;
                
                USE_SERIAL.printf("Hue: %d\n", colorTarget.h);


            }

            else if(payload[0] == 's') {

                // decode rgb data
                colorTarget.s = (uint32_t) strtol((const char *) &payload[1], NULL, 16);
                colorSet = colorLED;
                colorRatio = 0.0;
                
                USE_SERIAL.printf("Sat: %d\n", colorTarget.s);


            }
            
            else if(payload[0] == 'b') {

                // decode rgb data
                colorTarget.v = (uint32_t) strtol((const char *) &payload[1], NULL, 16);
                colorSet = colorLED;
                colorRatio = 0.0;
                
                USE_SERIAL.printf("Val: %d\n", colorTarget.v);


            }
            else if(payload[0] == 'w') {

                // Set effect mode 
                if (payload[1] == '1') waveEffect = true;
                else if (payload[1] == '0') waveEffect = false;
                
                USE_SERIAL.printf("Wave effect: %b\n", waveEffect);


            }
            webSocket.broadcastTXT(payload, length);

            break;
    }

}

void setup() {
    //USE_SERIAL.begin(921600);
    USE_SERIAL.begin(115200);

    USE_SERIAL.setDebugOutput(false);

    colorTarget = CHSV(EEPROM_ESP8266_LEER(0, 32).toInt(), EEPROM_ESP8266_LEER(32, 64).toInt(), EEPROM_ESP8266_LEER(64, 96).toInt());
    if (EEPROM_ESP8266_LEER(96, 128) == "1") waveEffect = true;
    else waveEffect = false;
    
    USE_SERIAL.println();
    USE_SERIAL.println();
    USE_SERIAL.println();

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    // Init LED Strip...
    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  
    // Launch test sequence at start
    testSequence();
  
    // Set default color
    fill_solid( leds, NUM_LEDS, colorLED);
  
    // Set initial brightness (OFF)
  
    // Display the initial color (BLACK)
    FastLED.show();

    WiFi.hostname("led");

    WiFiMulti.addAP(SSID_HOME, PWD_HOME);

    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }

    // Set Static IP
    IPAddress ip(192,168,0,185);   
    IPAddress gateway(192,168,0,1);   
    IPAddress subnet(255,255,255,0);   
    WiFi.config(ip, gateway, subnet);

   Serial.println();
   Serial.print("MAC: ");
   Serial.println(WiFi.macAddress());

    // start webSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    if(MDNS.begin("esp8266")) {
        USE_SERIAL.println("MDNS responder started");
    }

    // handle index
    server.on("/", []() {
      server.send(200, "text/html", html);
    });

    server.begin();

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);

}

void loop() {

  // Save persistent data
  if (millis() - saveStamp > saveTimer) {

    saveStamp = millis();

    char hTarget [32];
    String hStr = String(colorTarget.h);
    hStr.toCharArray(hTarget, 32);
    EEPROM_ESP8266_GRABAR(hTarget, 0); //Primero de 0 al 32, del 32 al 64, etc

    char sTarget [32];
    String sStr = String(colorTarget.s);
    sStr.toCharArray(sTarget, 32);
    EEPROM_ESP8266_GRABAR(sTarget, 32); //Primero de 0 al 32, del 32 al 64, etc
    
    char vTarget [32];
    String vStr = String(colorTarget.v);
    vStr.toCharArray(vTarget, 32);
    EEPROM_ESP8266_GRABAR(vTarget, 64); //SAVE

    char wTarget [32];
    String wStr = String(waveEffect);
    wStr.toCharArray(wTarget, 32);
    EEPROM_ESP8266_GRABAR(wTarget, 96); //SAVE

    USE_SERIAL.println("Data saved into EEPROM");
    USE_SERIAL.println(wTarget);
    
            
  }

  if ((millis() - timeStamp > fps) && (true)) {

    timeStamp = millis();
    // needsUpdate = false;

    
    // Check changes in color
    colorLED.h = lerp(colorSet.h, colorTarget.h, colorRatio);
    colorLED.s = lerp(colorSet.s, colorTarget.s, colorRatio);
    colorLED.v = lerp(colorSet.v, colorTarget.v, colorRatio);
    
    if (colorRatio >= 1.0) {
  
      colorSet = colorTarget;
      colorLED = colorTarget;
      colorRatio = 1.0;
    }
  
    else {
      colorRatio += deltaAnim;
    }

    // Apply effect if needed
    CHSV colorEffect = colorLED;

    if (waveEffect) {

      float ampH = 5.0;
      float ampV = 10.0;
      float seed = 1000.0;

      float stepH = millis() * 0.00008;
      float stepV = millis() * 0.00004;
      float newH = colorEffect.h + ampH * float(sn.noise(stepH, seed));
      if (newH > 255) newH = 255;
      if (newH < 0) newH = 0;
      
      float newV = colorEffect.v + ampV * float(sn.noise(stepV, 2 * seed));
      if (newV > 255) newV = 255;
      if (newV < 0) newV = 0;

      colorEffect.h = newH;
      colorEffect.v = newV;
      
      USE_SERIAL.print(colorEffect.h);
      USE_SERIAL.print(" ");
      USE_SERIAL.println(colorEffect.v);
      
    }
  
    // Apply changes and show lights
    fill_solid( leds, NUM_LEDS, colorEffect );
    FastLED.show();
  
  }

  // Handle network events
  webSocket.loop();
  server.handleClient();
}

// Initial Test Sequence (R - G - B)
void testSequence() {

  int tDelay = 20;
  int tDecay = 255;
  
  for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
    // Turn our current led on to white, then show the leds
    leds[whiteLed].setRGB(255, 0, 0);
    FastLED.show();
    delay(tDelay);
    leds[whiteLed].setRGB(0, 0, 0);

  }

    for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
    // Turn our current led on to white, then show the leds
    leds[whiteLed].setRGB(0, 255, 0);
    FastLED.show();
    delay(tDelay);
    leds[whiteLed].setRGB(0, 0, 0);

  }

  for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
    // Turn our current led on to white, then show the leds
    leds[whiteLed].setRGB(0, 0, 255);
    FastLED.show();
    delay(tDelay);
    leds[whiteLed].setRGB(0, 0, 0);

  }

}


float lerp( float start, float end, float step)
{
    return (end - start) * step + start;
}

void EEPROM_ESP8266_GRABAR(String buffer, int N) {
  EEPROM.begin(512); delay(10);
  for (int L = 0; L < 32; ++L) {
    EEPROM.write(N + L, buffer[L]);
  }
  EEPROM.commit();
}
//
String EEPROM_ESP8266_LEER(int min, int max) {
  EEPROM.begin(512); delay(10); String buffer;
  for (int L = min; L < max; ++L)
    if (isAlphaNumeric(EEPROM.read(L)))
      buffer += char(EEPROM.read(L));
  return buffer;
}
