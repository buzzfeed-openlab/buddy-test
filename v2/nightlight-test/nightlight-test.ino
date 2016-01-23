// Buddy nightlight test
// This is a discrete test for the night light function, in which buddy's light varies with the amount of light he perceives.

#include "math.h"
#include "neopixel.h"

#define PIXEL_PIN D1
#define PIXEL_COUNT 1
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// math values
float pi = 3.14;

// eyeball values
int powerPin=D7;
int eyeLeft=A1;
int eyeRight=A2;
int left; // value of left eye
int right; // value of right eye
int avg; // left and right values averaged
int lightmin = 0; // value of complete darkness
int lightmax = 300; // value of ambiently bright light

// brightness smoothing values, defined to start but subject to change
int b = 90;
int b0 = 90;
int period = 6000;

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

  int x = getBrightness();
  int t = millis();

  if (t%period == 0) {
    b0=x;
  }

  b=b0+(x-b0)*t/period;

  int red=b*rbase/100;
  int grn=b*gbase/100;
  int blu=b*bbase/100;
  colorAll(strip.Color(b*red, b*grn, b*blu), 0);

  Serial.print(b);
  Serial.print("    ");
  Serial.println(x);

}

/* whatever this doesn't work and is also dumb
int *smooth(int x, int t, int avg, int period, int h0, int h2) {
  // smooths out a variable by averaging this value over a certain period
  // get the average for every time t and push the value towards that in a cosine curve over the next interval.
  // variables needed:
  // period = period
  // t = time (millis)
  // a = amplitude
  // b = where the axis of the curve is centered-- should be h0+diff/2
  // diff = difference between original point x0 and future point h2.
  // h0 = starting point for curve, should be h at the end of last period
  // h2 = ending point for curve, should be the avg from the last period
  // h = current point, to be output.
  // avg = the average value, to be kept for resetting h2.
  // x = the current incoming value, to be added to the average.

  // recalibrate b
  int b = h0 + (h2-h0)/2;
  // recalibrate a
  int a = (h2-h0)/2;

  int h = b + a*cos(pi*t/(period));

  // get averages
  avg = (avg+x)/2;

  // find out if we're at interval

  if (t % period == 0) {
    // we are at interval
    // replace h0
    h0 = h2;
    // replace h2
    h2 = avg;
    // replace avg
    avg = x;
  }

  int r [4] = {h, avg, h0, h2};
  return r;

}
*/

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
