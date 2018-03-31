
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
int brightness = 0;
float fps = 1000.0 / 30;
CRGB colorLED;

File myFile, root;


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Serial port opened...");

  // Inicializamos y apagamos los LEDS de inicio...
  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);

  initSequence();
  
  for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed = whiteLed + 1) {
    // Turn our current led on to white, then show the leds
    leds[whiteLed].setRGB(255, 100, 40);          
  }

  FastLED.setBrightness(brightness);
          
  FastLED.show();
             
    // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

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

  if (lampON) brightness++;
  if (brightness > 255) brightness = 255;

  if (!lampON) brightness--;
  if (brightness < 0) brightness = 0;

  Serial.println("LED Status: " + String(lampON));

  FastLED.setBrightness(brightness);
  FastLED.show();
  FastLED.delay(fps);
                       
  EthernetClient client = server.available(); //Creamos un cliente Web
  //Cuando detecte un cliente a través de una petición HTTP
  if (client) {
    Serial.println("new client");
    boolean currentLineIsBlank = true; //Una petición HTTP acaba con una línea en blanco
    String cadena=""; //Creamos una cadena de caracteres vacía
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();//Leemos la petición HTTP carácter por carácter
        Serial.write(c);//Visualizamos la petición HTTP por el Monitor Serial
        cadena.concat(c);//Unimos el String 'cadena' con la petición HTTP (c). De esta manera convertimos la petición HTTP a un String
 
         //Ya que hemos convertido la petición HTTP a una cadena de caracteres, ahora podremos buscar partes del texto.
         int posicion=cadena.indexOf("LED="); //Guardamos la posición de la instancia "LED=" a la variable 'posicion'
 
          if(cadena.substring(posicion)=="LED=ON")//Si a la posición 'posicion' hay "LED=ON"
          {
            lampON=true;
            Serial.println("Encendemos");            
          }
          if(cadena.substring(posicion)=="LED=OFF")//Si a la posición 'posicion' hay "LED=OFF"
          {
            lampON=false;
            Serial.println("Apagamos");
          
          }
         
         // KIKE: Aquí faltaría convertir el texto en color y aplicarlo.
          
        //Cuando reciba una línea en blanco, quiere decir que la petición HTTP ha acabado y el servidor Web está listo para enviar una respuesta
        if (c == '\n' && currentLineIsBlank) {
 
            // Enviamos al cliente una respuesta HTTP
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();

            // Enviamos la pagina html
            if (myFile) {
              while(myFile.available()) {
                client.write(myFile.read()); // send web page to client
              }
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
    //Dar tiempo al navegador para recibir los datos
    delay(1);
    client.stop();// Cierra la conexión
  }
}

void initSequence() {

  int tDelay = 20;
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


