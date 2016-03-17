/* io-buddy.ino
----------------
Firmware for Buddy's first simple input-to-output emotional anchoring program.
This involves inputs being directly translated to sensory and haptic outputs
for Buddy's user.
Buddy has several senses that map to outputs the user in turn can sense.

Buddy's senses for the purpose of this firmware include:
0  Time: Buddy knows what time it is
1  Touch: Buddy knows how hard he is being squeezed
2  Light: Buddy senses the amount of light through a photoresistor
3  Temp: Buddy knows the temperature of the room, or at least of his sensor
4  Sound: Buddy knows how loud the room is (but does not record actual sound)

Buddy's outputs, which can be hooked individually to any of the inputs, include:
0  Vibration intensity: amplitude of the vibration
1  Vibration pattern: pattern (period) of the vibration
2  Light intensity: brightness of the rgb led
3  Light pattern: pattern (rapidness of "breathing") of the rgb led
4  Light color: color of the rgb led

An example of hooking up inputs and outputs might include:
  input   |   output
  -------------------
  Time    |   light color
  Touch   |   vib intensity
  Light   |   light intensity
  Temp    |   vib pattern
  Sound   |   light pattern

How do we hook these things up to each other?
You can call a function the command line that hooks up one ID to another ID.
The hookup listed above would be: 3,0,2,1,4

*/

#include "math.h"
/*#include "DS18B20.h"*/
#include "Particle-OneWire.h"
#include "neopixel.h"

#define PIXEL_PIN D1
#define LED_COUNT 1
#define LED_TYPE WS2812B

#define VIB_PIN D0
#define LED_PIN D1
#define TOUCH_PIN A0
#define LIGHT_PIN A1
/*#define TEMP_PIN A2*/
#define SOUND_PIN A3

float pi=3.14;            // pi. duh.
float e = 2.72;           // and his friend e.

// temperature values-- forget this, use weather api isntead
/*DS18B20 ds18b20 = DS18B20(A2); //Sets Pin A2
int led = D7;
double celsius;
double fahrenheit=0;
int DS18B20nextSampleTime;
int DS18B20_SAMPLE_INTERVAL = 2500;
int dsAttempts = 0;*/

//  Here are some math values for the sin function
int vibNum=0;   // lets you know which cycle you are on
int vibTotal=1;   // number of distinct vibration cycles we have
int vibT=0;                  // current time elapsed (starts at 0)
int vibLastT=0;
int vibPeriod [6] ={2000};          // seconds per cycle of the sin function
int vibMaxPower [6] ={50};          // highest y value of the function, which gets reset
int vibMidPower;  // amplitude! Set to vibMaxPower[r]/2 in every loop
int vibThreshold [6] ={50};         // y displacement of the sin function-- a power threshold added for vibration to be felt

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, LED_TYPE);

// Light cycle values
int ledNum=0;   // lets you know which cycle you are on
int ledTotal=2;   // total number of values in the LED array
int ledT=0;                  // current time elapsed (starts at 0)
int ledLastT=0;
int ledPeriod [6] ={2000,1000};          // seconds per cycle of the sin function
int ledMaxPower [6] ={50,50};          // highest y value of the function, which gets reset
int ledMidPower;  // amplitude! Set to ledMaxPower[r]/2 in every loop
int ledThreshold [6] ={50,50};         // y displacement of the sin function-- a power threshold added for vibration to be felt
int ledR [6] ={255,255};
int ledG [6] ={255,255};
int ledB [6] ={255,255};

// input values, all are scaled as x/1000
time_t timeVal;
int touchVal;
int lightVal;
int tempVal;
int soundVal;
// there are several weather values we keep track of
int precipVal [24]; // hourly percent likelihood of precipitation
int rainVal [24];  // hourly value of if it is rain (0), sleet (1), or snow (2)
int lightEstimate [24];  // value that predicts brightness level of outside (x/1000) as read by photoresistor


// temperature input calibrations
int tempMax=100;    // the maximum temperature we expect in F
int tempMin=0;    // the minimum temperature we expect in F


//  configuration values
int cellConfig=1;
int pubConfig=1;

void setup() {
  Time.zone(-8);
  Particle.syncTime();

  // Declare pin modes
  pinMode(VIB_PIN,OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(LIGHT_PIN, INPUT);
  /*pinMode(TEMP_PIN, INPUT);*/
  pinMode(SOUND_PIN, INPUT);

  strip.show(); // Initialize all pixels to 'off'

}

void loop() {

  // get all the input values
  // time
  int currentTime=Time.now();
  timeVal = getTimeVal(currentTime);
  parseTime(timeVal,3,0);
  // touch
  touchVal = analogRead(TOUCH_PIN);

  // light
  lightVal = analogRead(LIGHT_PIN);

  // temperature
  /*if (millis() > DS18B20nextSampleTime){
    getTemp();
    tempVal = fahrenheit;
  }*/

  // sound
  soundVal = analogRead(SOUND_PIN);

  // figure out how things are hooked up
  // make waves for light and vibration
  // do those things
}

void vibrate() {

  if (vibT<vibLastT) {  // if it is less than the last time, we're in a new cycle
    // switch to the next ledNum
    vibNum++;
    if (vibNum=vibTotal) {
      vibNum=0;
      // hypothetically this can also be used as a marker to
      // count or recalibrate based on sensors between periods
    }
  }

  vibT=millis()%vibPeriod[vibNum];
  int y = vibThreshold[vibNum] + vibMidPower + vibMidPower*sin(2*pi*vibT/vibPeriod[vibNum]);
  analogWrite(VIB_PIN,y);
}

void lightUp() {

  ledT=millis()%ledPeriod[ledNum];

  if (ledT<ledLastT) {  // if it is less than the last time, we're in a new cycle
    // switch to the next ledNum
    ledNum++;
    if (ledNum=ledTotal) {
      ledNum=0;
      // hypothetically this can also be used as a marker to
      // count or recalibrate based on sensors between periods
    }
  }

  ledMidPower=ledMaxPower[ledNum]/2;
  int y = ledThreshold[ledNum] + ledMidPower + ledMidPower+sin(2*pi*ledT/ledPeriod[ledNum]);
  // scale y by power range
  int rScale = scale(y,ledThreshold[ledNum],ledThreshold[ledNum]+ledMidPower, 0,ledR[ledNum]);
  int gScale = scale(y,ledThreshold[ledNum],ledThreshold[ledNum]+ledMidPower, 0,ledG[ledNum]);
  int bScale = scale(y,ledThreshold[ledNum],ledThreshold[ledNum]+ledMidPower, 0,ledB[ledNum]);

  strip.setPixelColor(0, strip.Color(rScale,gScale,bScale));
  strip.show();

  ledLastT=ledT;

}

int getTimeVal(time_t time) {

  // minutes should be mapped to the domain of a sin function
  // get the y value and use that to interpret

  int tx = Time.hour(time)+Time.minute(time);

  int recal = 1000*sin(2*pi*tx/1440);

  return recal;

}

// parseTime function tells you what to do with the values when they come in

void parseTime(int val, int pos, int cycle) {
  // takes in original value and intended output
  // input time value should be divided by 1000 to rescale
  if (pos==0) { // vibration intensity
    // transmit to vibMaxPower
    vibMaxPower[cycle]=val*vibMaxPower[cycle]/1000;
    // adjust vibMinPower
    vibMidPower=vibMaxPower[cycle]/2;
  }
  else if (pos==1) { // vibration pattern
    // scale by vibPeriod
    vibPeriod[cycle]=val*vibPeriod[cycle]/1000;
  }
  else if (pos==2) {  // light intensity
    // transmit to ledMaxPower
    ledMaxPower[cycle]=val*ledMaxPower[cycle]/1000;
    // adjust ledMidPower
    ledMidPower=ledMaxPower[cycle]/2;
  }
  else if (pos==3) { // light pattern
    // scale by ledPeriod
    ledPeriod[cycle]=val*ledPeriod[cycle]/1000;
  }
  else { // must be pos==4
    // light color
    // matters greatly on input

    // red should be max at noon
    ledR[cycle]=val*255/1000;

    // grn should come up to half of red.
    ledG[cycle]=val*123/1000;

    // blu should be max at midnight
    ledB[cycle]=255-val*255/1000;

  }
}

void getWeather(){

// hit the weather api, get weather.
// Maybe get this for the day at midnight every day
// and then put all the values in so you only have to hit the api once a day.
// put everything into arrays based on hour
// precipVal, rainVal, and lightEstimate

// probably should just do blue for rain and add red when warm rain and green when snow
// pattern for the weather color should probably hook up to light so that the weather color is longer
// if our light level is consistent with outside




}

int getTemp(int temp){

// hit the weather api, get temp

// we are going to scale so that higher temperature means higher value

  int tempr = restrict(temp,tempMin,tempMax);
  int value = scale(tempr,tempMin,tempMax,0,1000);

  // should use this to scale the ledR that hooks up with temperature

  return value;

}

/*void getTemp(){
    if(!ds18b20.search()){
      ds18b20.resetsearch();
      celsius = ds18b20.getTemperature();
      Serial.println(celsius);
      while (!ds18b20.crcCheck() && dsAttempts < 4){
        Serial.println("Caught bad value.");
        dsAttempts++;
        Serial.print("Attempts to Read: ");
        Serial.println(dsAttempts);
        if (dsAttempts == 3){
          delay(1000);
        }
        ds18b20.resetsearch();
        celsius = ds18b20.getTemperature();
        continue;
      }
      dsAttempts = 0;
      fahrenheit = ds18b20.convertToFahrenheit(celsius);
      DS18B20nextSampleTime = millis() + DS18B20_SAMPLE_INTERVAL;
      Serial.println(fahrenheit);
    }
}*/

// add parseTemp function that tells you what to do with the values when they come in

// add parseTouch function that tells you what to do with the values when they come in

// add parseLight function that tells you what to do with the values when they come in

// add parseSound function that tells you what to do with the values when they come in

//sets high and low bounds for an integer x
int restrict(int x, int min, int max) {
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

int scale(int x, int xmin, int xmax, int min, int max) {
  int scaled = min+max*(x-xmin)/(xmax-xmin);
  return scaled;
}

/*
int setVibIntensity(String command) {
  char inputStr[64];
  command.toCharArray(inputStr,64);
  char *p = strtok(inputStr,",");
  int newMaxPower = atoi(p);
  p = strtok(NULL,",");
  int newThreshold = atoi(p);
  vibMaxPower=newMaxPower;
  vibMidPower=newMaxPower/2;
  vibThreshold=newThreshold;
}

int setVibPattern(String command) {
  char inputStr[64];
  command.toCharArray(inputStr,64);
  int newPeriod = atoi(inputStr);
  vibPeriod=newPeriod;
}

int setLedIntensity(String command) {
  char inputStr[64];
  command.toCharArray(inputStr,64);
  char *p = strtok(inputStr,",");
  int newMaxPower = atoi(p);
  p = strtok(NULL,",");
  int newThreshold = atoi(p);
  ledMaxPower=newMaxPower;
  ledMidPower=newMaxPower/2;
  ledThreshold=newThreshold;
}

int setLedColor(String command) {
  char inputStr[64];
  command.toCharArray(inputStr,64);
  char *p = strtok(inputStr,",");
  int red = atoi(p);
  p = strtok(NULL,",");
  int grn = atoi(p);
  p = strtok(NULL,",");
  int blu = atoi(p);
  ledR=red;
  ledG=grn;
  ledB=blu;
}

int setLedPattern(String command) {
  char inputStr[64];
  command.toCharArray(inputStr,64);
  int newPeriod = atoi(inputStr);
  ledPeriod=newPeriod;
}*/

int wave(int t, int threshold, int period, int midPower) {
  return threshold+midPower+midPower*sin(2*pi*t/period);
}
