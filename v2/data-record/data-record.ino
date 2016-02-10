// Recording data locally with the SdFat library by grieman
// https://github.com/greiman/SdFat-Particle

#include "math.h"
#include "SdFat.h"

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

File myFile;

int vibpin=D0;
int powerPin=D6;
int squeezePin=A0;

// for testing
int analogvalue;
int valuevar=10; // the amount of acceptable variation in analogvalue before triggering a Serial print
int lastvalue=0;
int diff;
int lastdiff;
int state;
int laststate;

int threshold=50; //power threshold added for vibration to be felt
int th=5; // the current level of base power, to be varied up to threshold.

// wave function definitions
int period=2000;
int t=0;
float pi=3.14;
int maxPower=50;
int midPower=maxPower/2;

// decay function definitions
int readperiod=3000; // the time over which we take and average readings
int baseline; // baseline, to be calculated every readperiod
int currentavg; // average, taken every readperiod, just in case
int Q = 1000; // "penalty" value, 1 s to start
int QT = 2000; // the Q value before actions begin
int decayrate = 1; // the rate at which the decay occurs naturally
// the thresholds of analogvalue needed to trigger an action
int squeezeThreshold=500;
int petThreshold=200;
int inactionThreshold=50;
// internal calculation values
//slope (multiplier)
int squeezeM = 2;
int petM = -1;
int inactionM = 1;
//intercept (base)
int squeezeB=0; // no base value
int petB=-500;
int inactionB=0; // no base value

int r=1;

int lastrecord=0;
int recthreshold=5000; // record on sd card every 5 seconds

// if photon:
// SYSTEM_MODE(SEMI_AUTOMATIC);


void setup() {

    // Get the time
    // if photon
//    WiFi.on();
    Time.zone(-8);
//    Time.setFormat(TIME_FORMAT_ISO8601_FULL);
    Particle.syncTime();
//    if photon
//    WiFi.off();

    Serial.begin(9600);

    pinMode(vibpin,OUTPUT);
    pinMode(powerPin, OUTPUT);
    pinMode(squeezePin, INPUT);

    digitalWrite(powerPin,HIGH);

    Particle.function("analogread",tinkerAnalogRead);
    Particle.function("vib",vibrate);

    if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
      sd.initErrorHalt();
    }

}


void loop() {

    int amp = amplitude();

 /*
    Serial.print(amp);
    Serial.print("    ");
    Serial.println(Q);
*/

    analogWrite(vibpin,amp);

    checkpublish("sl");
    checkpublish("sd");

}

void checkpublish(String command) {

  if (command=="serial" || command=="sl") {
    Serial.print(analogvalue);
    Serial.print("    ");
    Serial.print(diff);
    Serial.print("    ");
    Serial.println(baseline);
  }
  else if (command=="publish" || command=="p") {
    char buddyskin[64];
    char bline[64];
    sprintf(buddyskin, "%2.2f", analogvalue);
    sprintf(bline, "%2.2f", baseline);
    if (abs(lastvalue-analogvalue)>valuevar && state!=laststate) {
      Particle.publish("librato_buddyskin",buddyskin);
      Particle.publish("librato_buddyavg",bline);
    }
  }
  else if (command=="SD" || command=="sd") {

    int millisNow=millis();
    time_t logTime=Time.now();

    // only record once every 5 seconds so you don't go crazy

    if ((millisNow-lastrecord)>recthreshold) {

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

      lastrecord=millisNow;

    }

  }

}

int vibrate(String command) {
    char inputStr[64];
    command.toCharArray(inputStr,64);
    char *p = strtok(inputStr,",");
    period = atoi(p);
    p = strtok(NULL,",");
    maxPower = atoi(p);
    midPower=maxPower/2;
    p = strtok(NULL,",");
    threshold = atoi(p);
}

int amplitude() {

    int y=0;

    // makes the vibration motor go based on what is going on

    // first integrate the penalty calculated (q) into the overall penalty (Q)
    // and subtract the current general decay

    analogvalue = analogRead(squeezePin);

    Q = Q + getpenalty(analogvalue) + decayrate;

    // Now use the Q to calculate the amplitude of the current wave

    if (Q<0) {
        t=millis();
        if (th<threshold) {
          th++;
        }
        y=th+midPower+midPower*sin(2*pi*t/period);
    }

    if (Q>0) {
      // ramp down the vibration by changing th within threshold
      if (th>0) {
        th--;
      }
      y=th+midPower+midPower*sin(2*pi*t/period);
    }

    if (Q<-10000) {
        Q=-10000;
    }
    else if (Q>10000) {
        Q=10000;
    }

    lastvalue=analogvalue;

    return y;

}

int getpenalty(int analogvalue) {
    // recalibrate the number based on current squeez-ed-ness

    // q is some value in ms that indicates the time penalty given for different actions on the toy

    // when q decays to 0 ms, there is maximum purr.
    // Starting at `maxPower` ms, you get some purr.
    // For values > `maxPower` ms, you have a decay rate of + or - depending on factors.

    // Every `readperiod` seconds, the analogvalue is averaged for a new `baseline`.
    // you get `diff` from the absolute value of the difference between the analogvalue and the baseline.

    // There is a range of values that can come in between baseline calculations, to either increase or inhibit the decay.

    // we can calculate the value of penalty q with
    // q=M*diff+B
    // where M is some pentalty multiplier and B is the base value
    // these values are unique per range for inaction, pet, and squeeze.

    // the value of q bottoms out at 0

    // take average of the last `readperiod` ms of readings and compare to current readings up until 1800, at which point average


    if ((millis() % readperiod)==0) {
        // if it is time to average the values, then average the values
        baseline=currentavg;
        currentavg=analogvalue;
    }
    else {
        // otherwise, update your average log
        currentavg=(currentavg+analogvalue)/2;
    }

    // get diff

    diff = abs(analogvalue-baseline);
    int M=0;
    int B=0;

    // inaction range
    if (diff<inactionThreshold) {
        M = inactionM;
        B = inactionB;
        laststate=state;
        state=0;
    }
    // pet range
    if (diff>inactionThreshold && diff < petThreshold) {
        M = petM;
        B = petB;
        laststate=state;
        state=1;
    }
    // squeeze range
    if (diff>squeezeThreshold) {
        M = squeezeM;
        B = squeezeB;
        laststate=state;
        state=2;
    }

    int q=M*diff+B;

//    Serial commands for testing
    Serial.print(M);
    Serial.print("   ");
    Serial.print(B);
    Serial.print("   ");
    Serial.print(diff);
    Serial.print("   ");
    Serial.print(q);
    Serial.print("   ");
    Serial.println(Q);
//*/

    lastdiff=diff;

    return q;

}

// added analogread for testing purposes

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
