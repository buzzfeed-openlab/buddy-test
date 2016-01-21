// Buddy nightlight test
// This is a discrete test for the night light function, in which buddy's light varies with the amount of light he perceives.

#include "math.h"
#include "neopixel.h"

#define PIXEL_PIN D1
#define PIXEL_COUNT 1
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

int powerPin=D7;
int eyeLeft=A1;
int eyeRight=A2;
int left; // value of left eye
int right; // value of right eye

void setup() {
  Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  pinMode(powerPin, OUTPUT);
  pinMode(eyeLeft, INPUT);
  pinMode(eyeLeft, INPUT);

  digitalWrite(powerPin,HIGH);

  Particle.function("set",setColor);
  Particle.function("see",eyesight);
}


int eyesight(String command) {
  if (command=="l" || command=="left") {
    left = analogRead(eyeLeft);
    return left;
  }
  else if (command=="r" || command=="right") {
    right = analogRead(eyeRight);
    return right;
  }
  else if (command=="a" || command == "average" || command == "avg") {
    left = analogRead(eyeLeft);
    right = analogRead(eyeRight);
    return (left+right)/2;
  }
  else {
    return -1;
  }
}

int setColor(String command) {
    char inputStr[64];
    command.toCharArray(inputStr,64);
    char *p = strtok(inputStr,",");
    int red = atoi(p);
    p = strtok(NULL,",");
    int grn = atoi(p);
    p = strtok(NULL,",");
    int blu = atoi(p);
    p = strtok(NULL,",");
    colorAll(strip.Color(red, grn, blu), 50);
}

// Set all pixels in the strip to a solid color, then wait (ms)
void colorAll(uint32_t c, uint8_t wait) {
  uint16_t i;

  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();
  delay(wait);
}
