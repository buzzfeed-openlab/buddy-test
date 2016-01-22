// This #include statement was automatically added by the Particle IDE.
#include "neopixel.h"
#include "math.h"

// IMPORTANT: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN D1
#define PIXEL_COUNT 1
#define PIXEL_TYPE WS2812B

#define squeezePin A0
#define eyePower D7
#define eyeLeft A4
#define eyeRight A5

int squeezeMax = 500;
int squeezeMin;

// recalibrated values
int redCal;
int grnCal;
int bluCal;

// eyesight values
int left;
int right;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);


void setup()
{
    Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  squeezeMin=analogRead(A0);
  digitalWrite(eyePower,HIGH);

  Particle.function("analogread", tinkerAnalogRead);
  Particle.function("set",setColor);

}

void loop()
{
// get how compressed it is
    // this gives us how squeezed it is. 100 is most compressed possible and 0 is not compressed at all.
    int squeezeRange=squeezeMax-squeezeMin;
    int analogvalue = analogRead(squeezePin);
    if (analogvalue > (1.5*squeezeMax)) {
        squeezeMax=analogvalue;
    }

    if (analogvalue<squeezeMin) {
        analogvalue=squeezeMin;
    }

    int redCal=255*(analogvalue-squeezeMin)/squeezeRange;
    int grnCal=255-255*(analogvalue-squeezeMin)/squeezeRange;
    int bluCal=255-255*(analogvalue-squeezeMin)/squeezeRange;
//    grnCal=155-(squeezeConst)*100;
//    bluCal=155-(squeezeConst)*100;

    eyesight();

    Serial.print(left);
    Serial.print("     ");
    Serial.println(right);

// translate that into the light pattern
    strip.setPixelColor(0, strip.Color(redCal,grnCal,bluCal));
    strip.setPixelColor(1, strip.Color(redCal,grnCal,bluCal));
    strip.setPixelColor(2, strip.Color(redCal,grnCal,bluCal));
    strip.setPixelColor(3, strip.Color(redCal,grnCal,bluCal));
    strip.show();
//    colorAll(strip.Color(redCal,grnCal,bluCal),50);

}

void eyesight() {
    left = analogRead(eyeLeft);
    right = analogRead(eyeRight);
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

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

int tinkerAnalogRead(String pin)
{
	//convert ascii to integer
	int pinNumber = pin.charAt(1) - '0';
	//Sanity check to see if the pin numbers are within limits
	if (pinNumber< 0 || pinNumber >7) return -1;

	if(pin.startsWith("D"))
	{
		return -3;
	}
	else if (pin.startsWith("A"))
	{
		return analogRead(pinNumber+10);
	}
	return -2;
}
