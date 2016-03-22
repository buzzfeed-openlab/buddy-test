/* io-test.ino
----------------
test firwmare for buddy's io to make sure all of it works
*/


#include "neopixel.h"
#include "math.h"

#define PIXEL_PIN D1
#define LED_COUNT 1
#define LED_TYPE WS2812B

#define VIB_PIN D0
#define LED_PIN D1
#define TOUCH_PIN A0
#define LIGHT_PIN A1
#define SOUND_PIN A2

float pi=3.14;            // pi. duh.
float e = 2.72;           // and his friend e.

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
int touchMin = 50;
int touchMax = 700;
int lightVal;
int lightMin = 0;
int lightMax = 500;
int soundVal;
int soundMin = 0;
int soundMax = 1500;

// there are several weather values we keep track of
int precipVal [24]; // hourly percent likelihood of precipitation
float snowVal [24];  // hourly value of if it is rain (0) or snow (1)
int lightEstimate [24];  // value that predicts brightness level of outside (x/1000) as read by photoresistor
int tempFeelVal [24];
int tempVal [24];
int tempMax=100;    // the maximum temperature we expect in F
int tempMin=0;    // the minimum temperature we expect in F
int lasthour=24;  // set to an impossible value to start

//  configuration values
int cellConfig=1;
int pubConfig=1;

void setup() {
  Time.zone(-7);
  Particle.syncTime();

  Serial.begin(9600);

  // Declare pin modes
  pinMode(VIB_PIN,OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(LIGHT_PIN, INPUT);
  pinMode(SOUND_PIN, INPUT);

  Particle.function("vib",vibrate);
  Particle.function("set",setColor);
  Particle.function("getWeather",getWeather);

  Particle.subscribe("hook-response/get_weather_SF", gotWeatherData, MY_DEVICES);

  strip.show(); // Initialize all pixels to 'off'

  // get weather for right now
  Particle.publish("get_weather_SF",PRIVATE);
}

void loop() {

  // get all the input values

  // time
  timeVal = getTime();
  parseTime(timeVal,3,0);
  /*Serial.print(timeVal);
  Serial.print("   ");*/

  // touch
  touchVal = getTouch();
  /*Serial.print(touchVal);
  Serial.print("   ");*/

  // light
  lightVal = getLight();
  /*Serial.print(lightVal);
  Serial.print("   ");*/

  // temperature and weather

  // sound
  soundVal = getSound();
  /*Serial.print(soundVal);
  Serial.print("   ");*/
  // figure out how things are hooked up
  // make waves for light and vibration
  // do those things

  int hourtime=Time.hour();
  if (hourtime!=lasthour) {
    if (hourtime==23) {
      getWeather("");
    }
    parseWeather(hourtime+1,4,1);
    lasthour=hourtime;
  }

//  vibrate();
  /*lightUp();*/
  /*Serial.println();*/
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

int getTime() {

  int currentTime=Time.now();

  // minutes should be mapped to the domain of a sin function
  // get the y value and use that to interpret

  int tx = Time.hour(currentTime)+Time.minute(currentTime);

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

int getWeather(String command) {
  Particle.publish("get_weather_SF",PRIVATE);
  for (int x; x<24; x++) {
    tempVal[x]=scale(tempVal[x],tempMin,tempMax,0,1000);
    tempFeelVal[x]=scale(tempFeelVal[x],tempMin,tempMax,0,1000);
    // snowVal already calibrated in gotWeatherData
    precipVal[x]=scale(precipVal[x],0,100,0,1000);
    lightEstimate[x]=precipVal[x]; // for now this can just be the precipitation likelihood but should make better algorithm later
  }

}

void gotWeatherData(const char *name, const char *data) {
  String str = String(data);

  char inputStr[256];
  str.toCharArray(inputStr,256);

  char *p;
  p = strtok(inputStr,"~");

  while (p != NULL)
  {
    int r=atoi(p);
    p = strtok (NULL, "~");
    tempVal[r]=atoi(p);
    p = strtok (NULL, "~");
    tempFeelVal[r]=atoi(p);
    p = strtok (NULL, "~");
    snowVal[r]=1000*atof(p);
    p = strtok (NULL, "~");
    precipVal[r]=atoi(p);
    p = strtok (NULL, "~");
    /*Serial.println(String(r));
    Serial.println(String(tempVal[r]));
    Serial.println(String(tempFeelVal[r]));
    Serial.println(String(snowVal[r]));
    Serial.println(String(precipVal[r]));*/
  }
/*
  for (int q=0; q<24; q++) {
    Serial.println(tempVal[q]);
  }*/
}

void parseWeather(int val, int pos, int cycle){
  // in this case, val = current hour

  if (pos==4) {
    // red should be max if hot
    ledR[cycle]=tempFeelVal[val]*255/1000;

    // grn should max if snow.
    ledG[cycle]=snowVal[val]*255/1000;

    // blu should be max at full rain
    ledB[cycle]=precipVal[val]*255/1000;

    /*strip.setPixelColor(0, strip.Color(ledR[cycle],ledG[cycle],ledB[cycle]));
    strip.show();*/
  }

// probably should just do blue for rain and add red when warm rain and green when snow
// pattern for the weather color should probably hook up to light so that the weather color is longer
// if our light level is consistent with outside

}

/*int parseTemp(int temp){

// hit the weather api, get temp

// we are going to scale so that higher temperature means higher value

  int x = restrict(temp,tempMin,tempMax);
  int value = scale(x,tempMin,tempMax,0,1000);

  // should use this to scale the ledR that hooks up with temperature

  return value;

}*/

// add parseTemp function that tells you what to do with the values when they come in

int getTouch() {
  int x = restrict(analogRead(TOUCH_PIN),touchMin,touchMax);
  int value = scale(x,touchMin,touchMax,0,1000);
  return value;
}
// add parseTouch function that tells you what to do with the values when they come in

int getLight() {
  int x = restrict(analogRead(LIGHT_PIN),lightMin,lightMax);
  int value = scale(x,lightMin,lightMax,0,1000);
  return value;
}
// add parseLight function that tells you what to do with the values when they come in

int getSound() {
  int x = restrict(analogRead(SOUND_PIN),soundMin,soundMax);
  int value = scale(x,soundMin,soundMax,0,1000);
  return value;
}

// add parseSound function that tells you what to do with the values when they come in

int vibrate(String command) {
    char inputStr[64];
    command.toCharArray(inputStr,64);
    char *p = strtok(inputStr,",");
    vibPeriod[0] = atoi(p);
    p = strtok(NULL,",");
    vibMaxPower[0] = atoi(p);
    vibMidPower=vibMaxPower[0]/2;
    p = strtok(NULL,",");
    vibThreshold[0] = atoi(p);
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


int restrict(int x, int min, int max) {
  //sets high and low bounds for an integer x
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
