
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


// Number of LEDS in the strip
#define NUM_LEDS 18

// Data pin that led data will be written out over
#define DATA_PIN 5

// Object representing the whole thing.
CRGB leds[NUM_LEDS];

// Network settings

// Enter a MAC address and IP address for your controller below.
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

// Lamp parameters
bool lampON = true;
int brightness = 0;
float fps = 1000.0 / 200.0;
CRGB colorLED;

// HTML File to be served
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
  for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
    // Turn our current led on to white, then show the leds
    leds[whiteLed].setRGB(255, 100, 40);          
  }

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
  if (lampON) brightness++;
  if (brightness > 255) brightness = 255;

  if (!lampON) brightness--;
  if (brightness < 0) brightness = 0;

  // Apply changes and show lights
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
 
         //Now we can search for patterns in the text.
         int posicion=cadena.indexOf("LED="); 

         // If "LED=ON" is found
          if(cadena.substring(posicion)=="LED=ON")
          {
            lampON=true;
          }
          // If "LED=OFF" is found
          if(cadena.substring(posicion)=="LED=OFF")//Si a la posición 'posicion' hay "LED=OFF"
          {
            lampON=false;
          
          }
         
         // TBD: Receive a color string and apply it.
          
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


