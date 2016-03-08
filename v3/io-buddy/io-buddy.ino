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
3  Light color: color of the rgb led
4  Light pattern: pattern (rapidness of "breathing") of the rgb led

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
#include "DS18B20.h"
#include "Particle-OneWire.h"

#define VIB_PIN D0
#define LED_PIN D1
#define TOUCH_PIN A0
#define LIGHT_PIN A1
#define TEMP_PIN A2
#define SOUND_PIN A3

float pi=3.14;            // pi. duh.
float e = 2.72;           // and his friend e.

// temperature values
DS18B20 ds18b20 = DS18B20(A2); //Sets Pin A2
int led = D7;
double celsius;
double fahrenheit=0;
int DS18B20nextSampleTime;
int DS18B20_SAMPLE_INTERVAL = 2500;
int dsAttempts = 0;

//  Here are some math values for the sin function
int vibPeriod=2000;          // seconds per cycle of the sin function
int vibT=0;                  // current time elapsed (starts at 0)
int vibMaxPower=50;          // highest y value of the function, which gets reset
int vibMidPower=vibMaxPower/2;  // amplitude!
int vibThreshold=50;         // y displacement of the sin function-- a power threshold added for vibration to be felt

// Light values
int ledPeriod=2000;          // seconds per cycle of the sin function
int ledT=0;                  // current time elapsed (starts at 0)
int ledMaxPower=50;          // highest y value of the function, which gets reset
int ledMidPower=ledMaxPower/2;  // amplitude!
int ledThreshold=50;         // y displacement of the sin function-- a power threshold added for vibration to be felt

// Color values
int ledR=255;
int ledG=255;
int ledB=255;

// input values
time_t timeVal;
int touchVal;
int lightVal;
int tempVal;
int soundVal;

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
  pinMode(TEMP_PIN, INPUT);
  pinMode(SOUND_PIN, INPUT);

}

void loop() {
  // get all the input values
  // time
  int currentTime=Time.now();
  timeVal = getTimeVal(currentTime);

  // touch
  touchVal = analogRead(TOUCH_PIN);

  // light
  lightVal = analogRead(LIGHT_PIN);

  // temperature
  if (millis() > DS18B20nextSampleTime){
    getTemp();
    tempVal = fahrenheit;
  }

  // sound
  soundVal = analogRead(SOUND_PIN);

  // figure out how things are hooked up
  // make waves for light and vibration
  // do those things
}

int getTimeVal(time_t time) {
  if (Time.hour(time)>=2 && Time.hour(time)<6) {
    return 1;
  }
  else if (Time.hour(time)>=6 && Time.hour(time)<10) {
    return 2;
  }
  else if (Time.hour(time)>=10 && Time.hour(time)<14) {
    return 3;
  }
  else if (Time.hour(time)>=14 && Time.hour(time)<18) {
    return 4;
  }
  else if (Time.hour(time)>=18 && Time.hour(time)<22) {
    return 5;
  }
  else if ((Time.hour(time)>=22 && Time.hour(time)<24) || (Time.hour(time)>=0 && Time.hour(time)<2)) {
    timeVal = 6;
  }
}

// add parseTime function that tells you what to do with the values when they come in

void getTemp(){
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
}

// add parseTemp function that tells you what to do with the values when they come in

// add parseTouch function that tells you what to do with the values when they come in

// add parseLight function that tells you what to do with the values when they come in

// add parseSound function that tells you what to do with the values when they come in


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
}

int wave(int t, int threshold, int period, int midPower) {
  return threshold+midPower+midPower*sin(2*pi*t/period);
}
