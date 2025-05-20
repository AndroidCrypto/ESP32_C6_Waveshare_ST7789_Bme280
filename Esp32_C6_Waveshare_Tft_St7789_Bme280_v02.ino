/*
  This sketch shows how to work with the ESP32-C6 Waveshare development board.
  Attached is an 1.47 inch TFT display that runs a ST7789 driver.
  The display has a size of 172x320 pixels.
  The library for the display is TFT_eSPI in a modified version, because the original
  version (2.5.43) can't run on ESP32 SDK 3.x so far and this SDK version is needed to
  select ESP32-C6 boards.

  Purpose of the sketch:
  - read the temperature, humidity and barometric air pressure from an attached BME280 sensor
  - display the temperature, humdity and air pressure on the TFT display with a Linear Analog Meter

  This is tested with ESP32 Board version 3.2.0 with SDK 5.4.1 on Arduino IDE 2.3.6
  TFT_eSPI version: 2.5.43
  Adafruit_BME280_Library version:  2.2.45
*/

/*
Version Management
27.04.2025 V02 Screen adapted on display size
23.04.2025 V01 Initial programming
*/

/*
TFT 172 x 320 pixels 1.47 inch ST7789 display wiring to an ESP32-C6 Waveshare
This is the factory setup that cannot be changed

TFT   ESP32-C6
GND   GND
VDD   3.3V 
SCL   7
SDA   6 (= "MOSI")
RST  21
DC   15
CS   14
BLK  22 *1)

Note *1) If you don't need a dimming you can connect BLK with 3.3V
Note *2) The display does not have a MISO ("output") terminal, so it is not wired
*/

// -------------------------------------------------------------------------------
// Sketch and Board information
const char *PROGRAM_VERSION = "ESP32-C6 Waveshare ST7789 BME280 V01";
const char *PROGRAM_VERSION_SHORT = "   ESP32-C6 ST7789 BME280";
const char *DIVIDER = "--------------------";

// -------------------------------------------------------------------------------
// TFT Display

#include <TFT_eSPI.h>     // Hardware-specific library
#include <TFT_eWidget.h>  // Widget library https://github.com/Bodmer/TFT_eWidget/tree/main
#include <SPI.h>

#define RED2RED 0
#define GREEN2GREEN 1
#define BLUE2BLUE 2
#define BLUE2RED 3
#define GREEN2RED 4
#define RED2GREEN 5
#define TFT_GREY 0x2104  // Dark grey 16 bit colour

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library with default width and height

#define TFT_BL_PIN 22              // backlight brightness control, needs to be a PWM pin
#define TFT_BRIGHTNESS_PERCENT 90  // avoids overheating of the device

const uint8_t linearMeterHeight = 50;
const uint8_t linearMeterStart = 15;

#define LOOP_PERIOD 35  // Display updates every 35 ms

// -------------------------------------------------------------------------------
// BME280 sensor

#define BME280_I2C_SDA_PIN 1
#define BME280_I2C_SCL_PIN 0
#define BME280_I2C_ADDRESS 0x76

#include <Wire.h>
#include <Adafruit_Sensor.h>  // https://github.com/adafruit/Adafruit_Sensor
#include <Adafruit_BME280.h>  // https://github.com/adafruit/Adafruit_BME280_Library
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;  // I2C

float temperature = -99;
float humidity = -99;
float pressure = -99;
float altitude = -99;

int interval = 2000;

void getBme280Values() {
  bme.takeForcedMeasurement();
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);

}

void printBme280Values() {
  Serial.print(F("Temperature: "));
  Serial.print(temperature, 1);
  Serial.print(F("c, Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%, Pressure: "));
  Serial.print(pressure, 0);
  Serial.print(F("hPa, Altitude: "));
  Serial.print(altitude, 1);
  Serial.println(F("m"));
  Serial.flush();
}

void setup(void) {
  Serial.begin(115200);
  delay(1000);
  Serial.println(PROGRAM_VERSION);

  tft.begin();

  // set the brightness to 90% to avoid heating of the device
  pinMode(TFT_BL_PIN, OUTPUT);
  analogWrite(TFT_BL_PIN, 255 * TFT_BRIGHTNESS_PERCENT / 100);
  delay(10);

  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  //tft.drawCentreString(PROGRAM_VERSION_SHORT, 64, 0, 1);
  tft.println(PROGRAM_VERSION_SHORT);

  bool wireStatus = Wire.begin(BME280_I2C_SDA_PIN, BME280_I2C_SCL_PIN);
  if (!wireStatus) {
    Serial.println(F("Wire begin failed"));
    while (1)
      ;
  } else {
    Serial.println(F("Wire begin running"));
  }

  delay(1000);
 
  bool bme_status = bme.begin(BME280_I2C_ADDRESS); // address either 0x76 or 0x77
  if (!bme_status) {

    Serial.println(F("No valid BME280 found"));
    while (1)
      ;
  } else {
    Serial.println(F("BME280 found"));
  }
  // the preferred way to get correct indoor temperatures
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X16,  // temperature
                  Adafruit_BME280::SAMPLING_X1,   // pressure
                  Adafruit_BME280::SAMPLING_X1,   // humidity
                  Adafruit_BME280::FILTER_X16,
                  Adafruit_BME280::STANDBY_MS_0_5);

}

void loop() {

  if ((millis() - interval) > 2000) {
    // Serial.println("Read BME280");
    getBme280Values();
    printBme280Values();

    float tempMap;
    tempMap = mapValue(temperature, (float)0.0, (float)30.0, (float)0.0, (float)20.0); // old n=15

    float humMap;
    humMap = mapValue(humidity, (float)0.0, (float)100.0, (float)0.0, (float)20.0);

    float presMap;
    presMap = mapValue(pressure, (float)900.0, (float)1100.0, (float)0.0, (float)20.0);

    char buffer[80];
    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    sprintf(buffer, " Temperature %.2f C ", temperature);
    tft.drawCentreString(buffer, 86, linearMeterStart, 1);
    linearMeterHelper(tempMap, 0, linearMeterStart + 10, BLUE2RED, " 0  5  10   15   20  25  30"); // 27 characters
    // the complete element is 50 pixels high, so next element should start + 50 in y coordinate

    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    sprintf(buffer, "Humidity %.1f vH", humidity);
    tft.drawCentreString(buffer, 86, linearMeterStart + linearMeterHeight, 1);
    linearMeterHelper(humMap, 0, linearMeterStart + linearMeterHeight + 10, GREEN2RED, " 0    25     50    75   100");

    tft.setTextColor(TFT_WHITE,TFT_BLACK);
    sprintf(buffer, "Air Pressure %.0f hPa", pressure);
    tft.drawCentreString(buffer, 86, linearMeterStart + 2 * linearMeterHeight, 1);
    linearMeterHelper(presMap, 0, linearMeterStart + 2 * linearMeterHeight + 10, RED2RED, " 900       1000        1100");

    // display the temperature in large letters
    tft.setTextColor(TFT_RED,TFT_BLACK);
    tft.drawCentreString(" Temperature ", 86, 165, 4);
    tft.setTextColor(TFT_GREEN,TFT_BLACK);
    sprintf(buffer, " %.1f C ", temperature);
    tft.drawCentreString(buffer, 86, 190, 4);

    // display the humidity in large letters
    tft.setTextColor(TFT_YELLOW,TFT_BLACK);
    tft.drawCentreString(" Humidity ", 86, 215, 4);
    tft.setTextColor(TFT_GREEN,TFT_BLACK);
    sprintf(buffer, " %.1f vH ", humidity);
    tft.drawCentreString(buffer, 86, 240, 4);

    // display the air pressure in small letters
    tft.setTextColor(TFT_BLUE,TFT_BLACK);
    tft.drawCentreString(" Air Pressure ", 86, 265, 4);
    tft.setTextColor(TFT_GREEN,TFT_BLACK);
    sprintf(buffer, " %.0f hPa ", pressure);
    tft.drawCentreString(buffer, 86, 290, 4);

    interval = millis();
  }
}

float mapValue(float ip, float ipmin, float ipmax, float tomin, float tomax) {
  return tomin + (((tomax - tomin) * (ip - ipmin)) / (ipmax - ipmin));
}

void linearMeterHelper(int val, int x, int y, byte colourScheme, char* scalaString) {
  linearMeter(val, x, y, 5, 20, 3, 20, colourScheme);
  //tft.drawLine(4, 102, 127, 102, TFT_WHITE);
  tft.drawLine(x + 4, y + 20 + 2, 171, y + 20 + 2, TFT_WHITE);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.drawString(scalaString, x + 3, y + 20 + 4);
}

// #########################################################################
// Return a 16 bit rainbow colour
// #########################################################################
unsigned int rainbow(byte value) {
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0;    // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;  // Green is the middle 6 bits
  byte blue = 0;   // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}

// #########################################################################
// Return a value in range -1 to +1 for a given phase angle in degrees
// #########################################################################
float sineWave(int phase) {
  return sin(phase * 0.0174532925);
}

// Linear Analog Meters

// #########################################################################
//  Draw the linear meter
// #########################################################################
// val =  reading to show (range is 0 to n)
// x, y = position of top left corner
// w, h = width and height of a single bar
// g    = pixel gap to next bar (can be 0)
// n    = number of segments
// s    = colour scheme
void linearMeter(int val, int x, int y, int w, int h, int g, int n, byte s) {
  // Variable to save "value" text colour from scheme and set default
  int colour = TFT_BLUE;
  // Draw n colour blocks
  for (int b = 1; b <= n; b++) {
    if (val > 0 && b <= val) {  // Fill in coloured blocks
      switch (s) {
        case 0: colour = TFT_RED; break;                             // Fixed colour
        case 1: colour = TFT_GREEN; break;                           // Fixed colour
        case 2: colour = TFT_BLUE; break;                            // Fixed colour
        case 3: colour = rainbowColor(map(b, 0, n, 127, 0)); break;  // Blue to red
        case 4: colour = rainbowColor(map(b, 0, n, 63, 0)); break;   // Green to red
        case 5: colour = rainbowColor(map(b, 0, n, 0, 63)); break;   // Red to green
        case 6: colour = rainbowColor(map(b, 0, n, 0, 159)); break;  // Rainbow (red to violet)
      }
      tft.fillRect(x + b * (w + g), y, w, h, colour);
    } else  // Fill in blank segments
    {
      tft.fillRect(x + b * (w + g), y, w, h, TFT_DARKGREY);
    }
  }
}

/***************************************************************************************
** Function name:           rainbowColor
** Description:             Return a 16 bit rainbow colour
***************************************************************************************/
// If 'spectrum' is in the range 0-159 it is converted to a spectrum colour
// from 0 = red through to 127 = blue to 159 = violet
// Extending the range to 0-191 adds a further violet to red band

uint16_t rainbowColor(uint8_t spectrum) {
  spectrum = spectrum % 192;

  uint8_t red = 0;    // Red is the top 5 bits of a 16 bit colour spectrum
  uint8_t green = 0;  // Green is the middle 6 bits, but only top 5 bits used here
  uint8_t blue = 0;   // Blue is the bottom 5 bits

  uint8_t sector = spectrum >> 5;
  uint8_t amplit = spectrum & 0x1F;

  switch (sector) {
    case 0:
      red = 0x1F;
      green = amplit;  // Green ramps up
      blue = 0;
      break;
    case 1:
      red = 0x1F - amplit;  // Red ramps down
      green = 0x1F;
      blue = 0;
      break;
    case 2:
      red = 0;
      green = 0x1F;
      blue = amplit;  // Blue ramps up
      break;
    case 3:
      red = 0;
      green = 0x1F - amplit;  // Green ramps down
      blue = 0x1F;
      break;
    case 4:
      red = amplit;  // Red ramps up
      green = 0;
      blue = 0x1F;
      break;
    case 5:
      red = 0x1F;
      green = 0;
      blue = 0x1F - amplit;  // Blue ramps down
      break;
  }

  return red << 11 | green << 6 | blue;
}
