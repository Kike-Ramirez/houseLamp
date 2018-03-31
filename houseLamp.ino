#include <SD.h>

/*
  WiFi Web Server

 A simple web server that shows the value of the analog input pins.
 using a WiFi shield.

 This example is written for a network using WPA encryption. For
 WEP or WPA, change the Wifi.begin() call accordingly.

 Circuit:
 * WiFi shield attached
 * Analog inputs attached to pins A0 through A5 (optional)

 created 13 July 2010
 by dlf (Metodo2 srl)
 modified 31 May 2012
 by Tom Igoe

 */

#include <SPI.h>
#include <Ethernet.h>
#include "FastLED.h"


// How many leds are in the strip?
#define NUM_LEDS 18

// Data pin that led data will be written out over
#define DATA_PIN 5

// Clock pin only needed for SPI based chipsets when not using hardware SPI
//#define CLOCK_PIN 8

// This is an array of leds.  One item for each led in your strip.
CRGB leds[NUM_LEDS];

// Network settings

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0F, 0x5A, 0x93
};
IPAddress ip(192, 168, 0, 185);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

bool lampON = true;

File myFile;


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Serial port opened...");
  
    // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());


  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);

  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  myFile = SD.open("index.html", FILE_READ);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Ok reading index.html...");

  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening index.html");
  }

}


void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
//          client.println("<!DOCTYPE html>");
//          client.println("<html>");
//          client.println("<body>");
//          client.println("<div>Hello World Kike</div>");
//          client.println("</body>");
//          
//          client.println("</html> ");

          if (myFile) {
            while(myFile.available()) {
              client.write(myFile.read()); // send web page to client
            }
            myFile.seek(0);
          }
          // Move a single white led 
          for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
             // Turn our current led on to white, then show the leds
             leds[whiteLed].setRGB(255, 100, 40);
        
           }
        
           FastLED.show();
           delay(100);
   
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disonnected");

    myFile.close();

    
  }
}



