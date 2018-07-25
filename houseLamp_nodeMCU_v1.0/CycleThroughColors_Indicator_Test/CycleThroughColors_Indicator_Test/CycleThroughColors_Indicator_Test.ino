
/* 
 * Example of using the ChainableRGB library for controlling a Grove RGB.
 * This code cycles through all the colors in an uniform way. This is accomplished using a HSB color space. 
 * comments add By: http://www.seeedstudio.com/
 * Suiable for Grove - Chainable LED 
 */
 
#include <Arduino.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ChainableLED.h>

int NUM_LEDS = 1;
int PIN_CLK = 19;
int PIN_DATA = 20;

float hue = 0.0;
boolean up = true;

ChainableLED leds(D2, D1, NUM_LEDS); //defines the pin used on arduino.

void setup()
{

  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("Running...");
  
}


void loop() {
  
  for (byte i=0; i < NUM_LEDS; i++) {
    
    Serial.println(hue);
    leds.setColorHSB(i, hue, 1.0, 0.1);
  
  }
    
  delay(50);
    
  if (up) hue+= 0.025;
  else hue-= 0.025;
    
  if (hue>=1.0 && up) up = false;
  else if (hue<=0.0 && !up) up = true;

}

