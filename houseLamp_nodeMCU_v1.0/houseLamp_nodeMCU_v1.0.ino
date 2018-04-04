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
float fps = 1.0 / 100.0;
float deltaAnim = 0.005;


// Initial color set
CRGB colorSet = CRGB(255, 100, 40);
CRGB colorLED = CRGB(255, 100, 40);
CRGB colorTarget = CRGB(255, 100, 40);
float colorRatio = 1.0;

// Initial brightness set
int brightness = 0;
int brightnessTarget = 255;
int brightnessSet = 0;
float brightnessRatio = 0.0;

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

            if(payload[0] == '#') {
                // we get RGB data

                // decode rgb data
                uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);

                analogWrite(LED_RED,    ((rgb >> 16) & 0xFF));
//                analogWrite(LED_GREEN,  ((rgb >> 8) & 0xFF));
//                analogWrite(LED_BLUE,   ((rgb >> 0) & 0xFF));
            }

            break;
    }

}

void setup() {
    //USE_SERIAL.begin(921600);
    USE_SERIAL.begin(115200);

    USE_SERIAL.setDebugOutput(true);

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
    FastLED.setBrightness(brightness);
  
    // Display the initial color (BLACK)
    FastLED.show();

    WiFiMulti.addAP(SSID_HOME, PWD_HOME);

    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
    }

    // start webSocket server
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    if(MDNS.begin("esp8266")) {
        USE_SERIAL.println("MDNS responder started");
    }

    // handle index
    server.on("/", []() {
        // send index.html
        server.send(200, "text/html", "<html><head><script>var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);connection.onopen = function () {  connection.send('Connect ' + new Date()); }; connection.onerror = function (error) {    console.log('WebSocket Error ', error);};connection.onmessage = function (e) {  console.log('Server: ', e.data);};function sendRGB() {  var r = parseInt(document.getElementById('r').value).toString(16);  var g = parseInt(document.getElementById('g').value).toString(16);  var b = parseInt(document.getElementById('b').value).toString(16);  if(r.length < 2) { r = '0' + r; }   if(g.length < 2) { g = '0' + g; }   if(b.length < 2) { b = '0' + b; }   var rgb = '#'+r+g+b;    console.log('RGB: ' + rgb); connection.send(rgb); }</script></head><body>LED Control:<br/><br/>R: <input id=\"r\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>G: <input id=\"g\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>B: <input id=\"b\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/></body></html>");
    });

    server.begin();

    // Add service to MDNS
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);

}

void loop() {

  // Check changes in brightness
  brightness = lerp(brightnessSet, brightnessTarget, brightnessRatio);

  if (brightnessRatio > 1.0) {

    brightnessSet = brightnessTarget;
    brightnessRatio = 1.0;
  }

  else brightnessRatio += deltaAnim;
  
  // Check changes in color

  colorLED.r = lerp(colorSet.r, colorTarget.r, colorRatio);
  colorLED.g = lerp(colorSet.g, colorTarget.g, colorRatio);
  colorLED.b = lerp(colorSet.b, colorTarget.b, colorRatio);
  
  if (colorRatio > 1.0) {

    colorSet = colorTarget;
    colorRatio = 1.0;
  }

  else colorRatio += deltaAnim;


  // Apply changes and show lights
  fill_solid( leds, NUM_LEDS, colorLED);
  FastLED.setBrightness(brightness);
  FastLED.show();

  // Delay according to required frame per seconds
  FastLED.delay(fps);

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
//    for(int j = 0; j < NUM_LEDS; j = j + 1) {
//      leds[j].r = leds[j].r - tDecay;
//      if (leds[j].r < 0) leds[j].r = 0;
//    }
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
