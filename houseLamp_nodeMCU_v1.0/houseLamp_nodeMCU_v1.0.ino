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


#define LED_RED     LED_BUILTIN

#define USE_SERIAL Serial

#define SSID_HOME "CarmenLauraKike"
#define PWD_HOME "Carmen2016"

// LAMP SETTINGS

// Frames per seconds and step for lerp animations
float fps = 1000.0 / 50.0;
float timeStamp = millis();

float deltaAnim = 0.04;
// bool needsUpdate = true;


// Initial color set
CHSV colorSet = CHSV(0, 0, 0);
CHSV colorLED = CHSV(0, 0, 0);
CHSV colorTarget = CHSV(0, 0, 255);
float colorRatio = 0.0;


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
String html ="<!DOCTYPE html> <html> <head> <meta charset=\"utf-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"> <link rel=\"apple-touch-icon\" href=\"https://www.kikeramirez.org/images/logo_kikeramirez_64.png\" /> <link rel=\"apple-touch-icon-precomposed\" href=\"https://www.kikeramirez.org/images/logo_kikeramirez_64.png\" /> <link rel=\"icon\" href=\"https://www.kikeramirez.org/images/logo_kikeramirez_64.png\"> <title>LED</title> <meta name=\"description\" content=\"LED Control Site.\"> <meta name=\"keywords\" content=\"creative coding, interaction, arduino, bootstrap, LED, javascript\"> <link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\" type=\"text/css\"> <link rel=\"stylesheet\" href=\"https://www.kikeramirez.org/assets/houseLamp/styleguide.html\"> <link rel=\"stylesheet\" href=\"https://www.kikeramirez.org/assets/houseLamp/theme.css\"> </head> <body class=\"bg-primary\"> <div class=\"py-5 text-center\" style=\"background-size: cover; background-image: url(&quot;http://www.2sega.com/wp-content/uploads/2016/04/LED-lights.jpg&quot;);\"> <div class=\"container py-5\"> <div class=\"row\"> <div class=\"col-md-12\"> <h1 class=\"display-3 mb-4 text-primary\">LED</h1> <p class=\"lead mb-5\"> LED Strip controlled via LAN using a Arduino-based web server.</p> <p class=\"text-secondary mb-5\"> K.Ramírez - April, 2018 - MIT License</p> <button type=\"button\" class=\"col-12 btn btn-lg btn-primary mx-1\" onclick =\"sendON();\">ON</button> <button type=\"button\" class=\"col-12 btn btn-lg btn-primary mx-1\" onclick =\"sendOFF();\">OFF</button> <input id=\"h\" name=\"Hue\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" value=\"0\" oninput=\"sendHue();\" class=\"col-12 btn btn-lg mx-1 btn-secondary\" /> <input id=\"s\" name=\"Saturation\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" value=\"0\" oninput=\"sendSaturation();\" class=\"col-12 btn btn-lg mx-1 btn-secondary\" /> <input id=\"v\" name=\"Brightness\" type=\"range\" min=\"5\" max=\"255\" step=\"1\" value=\"255\" oninput=\"sendBrightness();\" class=\"col-12 btn btn-lg mx-1 btn-secondary\" /> </div> </div> </div> </div> <script src=\"https://code.jquery.com/jquery-3.2.1.slim.min.js\" integrity=\"sha384-KJ3o2DKtIkvYIK3UENzmM7KCkRr/rE9/Qpg6aAZGJwFDMVNA/GpGFF93hXpG5KkN\" crossorigin=\"anonymous\"></script> <script src=\"https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js\" integrity=\"sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q\" crossorigin=\"anonymous\"></script> <script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js\" integrity=\"sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl\" crossorigin=\"anonymous\"></script> <script> var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']); var brightnessRange = document.getElementById(\"v\"); connection.onopen = function () { connection.send('Connect ' + new Date()); }; connection.onerror = function (error) { console.log('WebSocket Error ', error); }; connection.onmessage = function (e) { console.log('Server: ', e.data); }; function sendHue() { var hue = 'h' + parseInt(document.getElementById('h').value).toString(16); console.log('Hue: ' + hue); connection.send(hue); }; function sendSaturation() { var sat = 's' + parseInt(document.getElementById('s').value).toString(16); console.log('Saturation: ' + sat); connection.send(sat); } function sendBrightness() { var bright = \"b\" + parseInt(document.getElementById('v').value).toString(16); console.log('Brightness: ' + bright); connection.send(bright); } function sendON() { var bright = \"bff\"; console.log('ON'); connection.send(bright); brightnessRange.value = 255; } function sendOFF() { var bright = \"b0\"; console.log('OFF'); connection.send(bright); brightnessRange.value = 0; } </script> </body> </html>";


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

            break;
    }

}

void setup() {
    //USE_SERIAL.begin(921600);
    USE_SERIAL.begin(115200);

    USE_SERIAL.setDebugOutput(false);

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
        // send index.html
//        server.send(200, "text/html", "<html><head><script>var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);connection.onopen = function () {  connection.send('Connect ' + new Date()); }; connection.onerror = function (error) {    console.log('WebSocket Error ', error);};connection.onmessage = function (e) {  console.log('Server: ', e.data);};function sendRGB() {  var r = parseInt(document.getElementById('r').value).toString(16);  var g = parseInt(document.getElementById('g').value).toString(16);  var b = parseInt(document.getElementById('b').value).toString(16);  if(r.length < 2) { r = '0' + r; }   if(g.length < 2) { g = '0' + g; }   if(b.length < 2) { b = '0' + b; }   var rgb = '#'+r+g+b;    console.log('RGB: ' + rgb); connection.send(rgb); }</script></head><body>LED Control:<br/><br/>R: <input id=\"r\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>G: <input id=\"g\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>B: <input id=\"b\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/></body></html>");
        server.send(200, "text/html", html);
    });

    server.begin();

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);

}

void loop() {

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
  
  
    // Apply changes and show lights
    fill_solid( leds, NUM_LEDS, colorLED);
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
