// Buddy nightlight test
// This is a discrete test for the night light function, in which buddy's light varies with the amount of light he perceives.

#include "math.h"
#include "neopixel.h"

#define PIXEL_PIN D1
#define PIXEL_COUNT 1
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// eyeball values
int powerPin=D7;
int eyeLeft=A1;
int eyeRight=A2;
int left; // value of left eye
int right; // value of right eye
int avg; // left and right values averaged
int lightmin = 0; // value of complete darkness
int lightmax = 300; // value of ambiently bright light

// timing values
int sunrise = 429;
int sunriseprebuffermin = 5;
int sunrisepostbuffermin = 5;
int sunset = 1110;
int sunsetprebuffermin = 5;
int sunsetpostbuffermin = 5;


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

  Time.zone(-8); //set time zone to PST
}

void loop() {

  // in order to smooth this out need to get the average incoming light every 20 seconds or so.
  // change the brightness based on this variable instead.

  avg=eyesight("avg");

  int rbase, gbase, bbase;

  int currenttime = 60*Time.hour()+Time.minute();

  rbase=0;
  gbase=255;
  bbase=255;
  int b = getBrightness();

  int red=b*rbase/100;
  int grn=b*gbase/100;
  int blu=b*bbase/100;
  colorAll(strip.Color(b*red, b*grn, b*blu), 0);

  Serial.print(red);
  Serial.print("    ");
  Serial.print(grn);
  Serial.print("    ");
  Serial.print(blu);
  Serial.print("    ");
  Serial.println(b);

  delay(50);

}

int smooth(int current, int last, int lastavg, int t) {
  // smooths out a variable by averaging this value over time t
  // take modulo of millis with t
  // if modulo not equal to 0 then can just add this value to the previous value and divide by 2
  // also will keep a lastavg value so that can smooth between averages a bit.
  // every interval t, we will reset lastavg and return avg
  // every noninterval, we will return avg and average last.
}

float getBrightness() {
  // calibrates the brightness of the strip based on the incoming light
  // the resulting float should be a percent of full brightness (255)
  // can calibrate these values later
  // remember that resulting value is an integer between 0 and 100.

  int val=bound(eyesight("avg"), lightmin, lightmax);
  int brightness = 100-100*val/lightmax;

  return brightness;
}

int bound(int x, int min, int max) {
  // if higher than max, set to max
  // if lower than min, set to min
  if (x>max) {
    return max;
  }
  else if (x<min) {
    return min;
  }
  else {
    return x;
  }
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
    avg = (left+right)/2;
    return avg;
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
