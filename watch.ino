#include <Wire.h>
#include <Adafruit_GFX.h>        //OLED libraries
#include <Adafruit_SSD1306.h>

#include "MAX30105.h"
#include "heartRate.h"
#include <stdlib.h>

MAX30105 particleSensor;

const byte RATE_SIZE = 8; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;

long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;
bool fingerFound = false;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 oled (SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const unsigned char PROGMEM logo2_bmp[] = { 
0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10,

0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,

0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,

0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00,  };

void setup() {
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Start the OLED display
  oled.display();
  delay(1000);

  Serial.begin(115200);
  Serial.println("Initializing...");

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  displayFingerOff(); // display the finger query screen upon start up
}

// display the BPM and a pretty logo
void displayFingerOn() {
  oled.clearDisplay();                                   
  oled.drawBitmap(5, 5, logo2_bmp, 24, 21, WHITE);       
  oled.setTextSize(2);                                   
  oled.setTextColor(WHITE); 
  oled.setCursor(50,0);                
  oled.println("BPM");               
  oled.setCursor(50,18);                
  oled.println(beatAvg);  
  oled.display();
  return;
}

// query the user to place their finger
void displayFingerOff() {
  oled.clearDisplay();                                   
  oled.setTextSize(2);                                   
  oled.setTextColor(WHITE); 
  oled.setCursor(0,0);                
  oled.println("Place your finger");             
  oled.display();
  return;
}

void loop() {
  long irValue = particleSensor.getIR();   
  
  // if the user has placed their finger when it was not previously resting
  if (irValue > 7000) {
    if (!fingerFound) {
      displayFingerOn();
    }
    fingerFound = true;
  }
  else { // otherwise, signal finger removed
    if (fingerFound) {      
      displayFingerOff();
    }
    fingerFound = false;
  }
  
  if (checkForBeat(irValue)) { // check for a heart beat in the user's finger
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);
    if (beatsPerMinute < 255 && beatsPerMinute > 20) { // filter out garbage values
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; // Wrap variable

      // Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
      Serial.println(beatAvg); // output beat average to console
      displayFingerOn(); // update the display with new value
    }
  }
}
