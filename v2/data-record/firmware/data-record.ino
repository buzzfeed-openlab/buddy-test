// Recording data locally with the SdFat library by grieman
// https://github.com/greiman/SdFat-Particle

#include "math.h"
#include "SdFat.h"

// Some definitions for the SD card
#define SPI_CONFIGURATION 0
//------------------------------------------------------------------------------
// Setup SPI configuration.
#if SPI_CONFIGURATION == 0
// Primary SPI with DMA
// SCK => A3, MISO => A4, MOSI => A5, SS => A2 (default)
SdFat sd;
const uint8_t chipSelect = SS;
#elif SPI_CONFIGURATION == 1
// Secondary SPI with DMA
// SCK => D4, MISO => D3, MOSI => D2, SS => D1
SdFat sd(1);
const uint8_t chipSelect = D1;
#elif SPI_CONFIGURATION == 2
// Primary SPI with Arduino SPI library style byte I/O.
// SCK => A3, MISO => A4, MOSI => A5, SS => A2 (default)
SdFatLibSpi sd;
const uint8_t chipSelect = SS;
#elif SPI_CONFIGURATION == 3
// Software SPI.  Use any digital pins.
// MISO => D5, MOSI => D6, SCK => D7, SS => D0
SdFatSoftSpi<D5, D6, D7> sd;
const uint8_t chipSelect = D0;
#endif  // SPI_CONFIGURATION
//------------------------------------------------------------------------------

// Pin Values
#define VIBPIN D0
#define POWERPIN D6
#define SQUEEZEPIN A0

// Purr Wave Values
//    We create the purr with a sin wave that increases power past a particular threshold in order to be felt by the user.
//    The purr starts or stops depending on penalty values (Q).
//    When Q<0, Buddy will ramp up his purr.
//    When Q>0, Buddy ramps down his purr.
//  Here are some math values for the sin function
int period=2000;          // seconds per cycle of the sin function
int t=0;                  // current time elapsed (starts at 0)
float pi=3.14;            // pi. duh.
int maxPower=50;          // highest y value of the function
int midPower=maxPower/2;  // amplitude!
int threshold=50;         // y displacement of the sin function-- a power threshold added for vibration to be felt
int th=5;                 // when we want to make people feel the vibration more or less, we will vary th up to threshold.
//  Here are the ways we actually determine whether he should purr or not.
int Q = 1000;             // Here's the Q penalty value. We start Q at 1000
int QMax = 10000;         // Upper bound of Q range
int QMin = -10000;        // Lower bound of Q range
//  Q is actually a cumulative value, an sum of the q penalty that occurs every loop.
//  We determine q by plugging diff into a linear equation.
//  q is positive or negative depending on the M (slope), B (intercept), and diff values.
//  M is a multiplier-- is effect is fully dependent on diff.
int squeezeM = 2;         // 2 * diff added to Q
int petM = -1;            // diff subtracted from Q
int inactionM = 1;        // diff added to Q
//   B is a base value that does not depend on the value of diff.
int squeezeB=0;           // no value added or subtracted from Q
int petB=-500;            // 500 subtracted from Q
int inactionB=0;          // no value added or subtracted from Q

// State Determination Values
//   things that help us define whether Buddy should purr or not.
//   The state is dependent on the difference between Buddy's analogvalue readings.
int analogvalue;          // the actual value of squeeze coming from SQUEEZEPIN
int readperiod=3000;      // the time over which we average analogvalue readings
int lastread=0;           // the time at which we finished our last baseline
int baseline;             // baseline from which we calculate diff, to be calculated every readperiod
int currentavg;           // average, taken every loop and factored into baseline at the end of the readperiod.
int diff;                 // the difference between analogvalue and baseline
//   State is determined by diff.
//   Here are the thresholds that we use to determine which state we are in.
int squeezeThreshold=350;
int petThreshold=150;
int inactionThreshold=10;
//   Here is the actual state.
int state;                // buddy's current state
                          //    0: just started up / returned to inaction state
                          //    1: stable in inaction state
                          //    2: just started being petted
                          //    3: still being petted
                          //    4: just started being squished
                          //    5: still being squished

// Recording Values
//    For recording purposes, we don't always want to publish the analogvalue.
//    Especially if using celluar, that would take up too much data.
//    Here's some stuff we do to restrict that
int publishflag=0;         // tells us if we should publish or not, based on the state
int lastpublish=0;         // gives time of last publish
int publishthreshold=5000; // the min time that must pass between publishes
int sdlastrecord=0;        // the last time we recorded to an SD card
int sdrecthreshold=1000;   // the amount of time that needs to pass between SD card recordings
File myFile;               // this is our SD card file variable

// FOR WI-FI ONLY (Photon)
// SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {
    // Get the time

    // FOR CELLULAR
    Time.zone(-8);
    Particle.syncTime();

    // FOR WI-FI ONLY (Photon)
    /*--------------------
    WiFi.on();
    Time.zone(-8);
    Particle.syncTime();
    WiFi.off();
    --------------------*/

    // Set up serial
    Serial.begin(9600);

    // Declare pin modes
    pinMode(VIBPIN,OUTPUT);
    pinMode(POWERPIN, OUTPUT);
    pinMode(SQUEEZEPIN, INPUT);

    // Turn on your power pin (this just gives us a constant level of power)
    // Could also just use 3v3 pin
    digitalWrite(POWERPIN,HIGH);

    // Start up your SD card
    if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
      sd.initErrorHalt();
    }
}


void loop() {
    // what time is it
    t = millis();

    // get analogvalue
    analogvalue = analogRead(SQUEEZEPIN);

    // get the diff
    diff = getDiff();

    // use the diff to calculate state
    state = getState();

    int q = doState();

    // use little q penalty to calculate big Q penalty
    // restrict between the QMin and QMax bounds
    Q = restrict(Q+q,QMin,QMax);

    // get the current value to be written to the vibration motor
    int y = getVibration();

    // write that value to the vibration motor
    analogWrite(VIBPIN,y);

//    recordSerial();
    recordSD();

    if (publishflag==1) {
      recordCloud();
    }

}

int getDiff() {
  // Get baseline if necessary
  if ((t-lastread) > readperiod) {
      baseline=currentavg;
      currentavg=analogvalue;
      lastread=t;
  }
  else {
      // If we haven't reached a readperiod, just update currentavg
      currentavg=(currentavg+analogvalue)/2;
  }

  // Now get diff
  // We are letting diff be the absolute value, since sometimes petting Buddy results in positive or negative differences from baseline.
  diff = abs(analogvalue-baseline);

  return diff;
}

int getState() {
  if (diff<inactionThreshold) {
    if (state==0 || state==1) {
      return 1;
    }
    else {
      return 0;
    }
  }
  if (diff>inactionThreshold && diff < petThreshold) {
    if (state==2 || state==3) {
      return 3;
    }
    else {
      return 2;
    }
  }
  if (diff>squeezeThreshold) {
    if (state==4 || state==5) {
      return 5;
    }
    else {
      return 4;
    }
  }
}

int doState() {
    // this is where we will actually get q for the state
    // we can do this by varying M and B in each state
    int M;
    int B;
    int q;

    if (state==0) {
      publishflag=1;
      M=0;
      B=0;
    }
    else if (state==1) {
      publishflag=0;
      M = inactionM;
      B = inactionB;
    }
    else if (state==2) {
      publishflag=1;
      M=0;
      B=0;
    }
    else if (state==3) {
      publishflag=0;
      M = petM;
      B = petB;
    }
    else if (state==4) {
      publishflag=1;
      M=0;
      B=0;
    }
    else if (state==5) {
      publishflag=0;
      M = squeezeM;
      B = squeezeB;
    }

    q=M*diff+B;
    return q;
}

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

int getVibration() {
    int y;

    if (Q<0) {
      // negative Q, ramp up the vibration
        if (th<threshold) {
          th++;
        }
        y=th+midPower+midPower*sin(2*pi*t/period);
    }

    if (Q>=0) {
      // ramp down the vibration by changing th within threshold
      if (th>0) {
        th--;
      }
      y=th+midPower+midPower*sin(2*pi*t/period);
    }

    return y;
}

void recordSerial() {
  Serial.print(state);
  Serial.print("    ");
  Serial.print(analogvalue);
  Serial.print("    ");
  Serial.print(diff);
  Serial.print("    ");
  Serial.println(baseline);
}

void recordSD() {
  time_t logTime=Time.now();

  if ((t-sdlastrecord)>sdrecthreshold) {
    // write to sd card
    if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
      sd.initErrorHalt();
    }

    // open the file for write at end like the "Native SD library"
    if (!myFile.open("buddylog.txt", O_RDWR | O_CREAT | O_AT_END)) {
      sd.errorHalt("opening buddylog.txt for write failed");
    }

    // if the file opened okay, write to it:
    myFile.print(Time.year(logTime));
    myFile.print("-");
    myFile.print(Time.month(logTime));
    myFile.print("-");
    myFile.print(Time.day(logTime));
    myFile.print("-");
    myFile.print(Time.hour(logTime));
    myFile.print(":");
    myFile.print(Time.minute(logTime));
    myFile.print(":");
    myFile.print(Time.second(logTime));
    myFile.print(",");
    myFile.print(analogvalue);
    myFile.print(",");
    myFile.println(baseline);

    // close the file:
    myFile.close();

    sdlastrecord=t;
  }
  if (sdlastrecord>t) {
    sdlastrecord=t;
  }
}

void recordCloud() {
  char val[64];
  char base[64];
  sprintf(val, "%d", analogvalue);
  sprintf(base, "%d", baseline);
  if ((t-lastpublish)>publishthreshold) {
    Particle.publish("librato_bVal",val,PRIVATE);
    Particle.publish("librato_bBase",base,PRIVATE);
    lastpublish=t;
  }
  // in case of values wrapping around
  if (lastpublish>t) {
    lastpublish=t;
  }
}