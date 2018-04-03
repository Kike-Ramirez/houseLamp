
/*
  >> houseLamp v0.1 <<

  Description: Arduino sketch controlling a LED Strip in a domestic environment. It acts as a web server in a LAN offering
  the user to control the LED Lamp remotely.

  Author: Kike Ramírez
  Date: 1/4/2018

  Web: www.kikeramirez.org
  Mail: info@kikeramirez.org
  
  License: MIT

 */

#include <SPI.h>
#include <Ethernet.h>
#include "FastLED.h"
#include <SD.h>


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
#define NUM_LEDS 18

// Arduino Data pin that led data will be written out over
#define DATA_PIN 5

// Object representing the whole strip.
CRGB leds[NUM_LEDS];

// NETWORK SETTINGS

// Enter a MAC address and IP address for your Ethernet Shield below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0F, 0x5A, 0x93
};

// Fixed IP Address
IPAddress ip(192, 168, 0, 185);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// HTML File (stored in SD card) to be served
File myFile;


void setup() {
  
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Serial port opened...");

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
             
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  // Initialize SD Card and get html file
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("INDEX~1.HTM");
  
  // if the file opened okay, write to it:
  while (!myFile) {
    Serial.println("Error reading index.html... Retry in 2 seconds...");
    delay(2000);

  } 
  
  Serial.println("Succesfully opened index.html");
  
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

  // Create a web client if available
  EthernetClient client = server.available(); 
  // If a client is detected
  if (client) {
    Serial.println("new client");
    // A HTTP request ends in a blank line
    boolean currentLineIsBlank = true;

    // Create an empty string
    String cadena=""; 
    
    while (client.connected()) {
      if (client.available()) {
        // Read the HTTP Request character by character
        char c = client.read();
        // Show request in serial
        Serial.write(c);
        // Join "cadena" with HTTP request to convert it to a string.
        cadena.concat(c);
 
         // Search for "LED=" pattern in the text.
         int posicion=cadena.indexOf("LED="); 

         // If "LED=ON" is found
          if(cadena.substring(posicion)=="LED=ON")
          {
            brightnessTarget = 255;
            brightnessRatio = 0.0;
          }
          
          // If "LED=OFF" is found
          if(cadena.substring(posicion)=="LED=OFF")
          {
            brightnessTarget = 0;
            brightnessRatio = 0.0;
          
          }

         // Search for "COLOR=" pattern in the text.
         int posicion2 = cadena.indexOf("COLOR="); 

         // Extract color value from text
          if(cadena.substring(posicion2).length() == 14 )
          {
            String colorHex = cadena.substring(posicion2 + 6);
            long long number = strtol( &colorHex[0], NULL, 16);

            // Split them up into r, g, b values
            long long r = number >> 16;
            long long g = number >> 8 & 0xFF;
            long long b = number & 0xFF;
            
            colorTarget = CRGB(r, g, b);
            colorRatio = 0.0;
 
          }
          
        //If a blank line is received HTTP request has ended (send response now!)
        if (c == '\n' && currentLineIsBlank) {
 
            // Send a HTTP Response
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();

            // Send a html file stored in SD Card
            if (myFile) {
              while(myFile.available()) {
                // send web page to client
                client.write(myFile.read()); 
              }
              // "Rewind" the file
              myFile.seek(0);
            }     
          break;
          
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }

    }

    // Give the browser some time to receive data
    delay(1);
    
    // Close connection
    client.stop();
  }
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

