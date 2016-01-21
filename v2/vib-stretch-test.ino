#include "math.h"

int vibpin=D0;
int powerPin=D7;
int squeezePin=A0;

int threshold=35; //power threshold added for vibration to be felt
int th=30; // the current level of base power, to be varied up to threshold.

// wave function definitions
int period=1500;
int t=0;
float pi=3.14;
int maxPower=25;
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


void setup() {

    Serial.begin(9600);

    pinMode(vibpin,OUTPUT);
    pinMode(powerPin, OUTPUT);
    pinMode(squeezePin, INPUT);

    digitalWrite(powerPin,HIGH);

    Particle.function("analogread",tinkerAnalogRead);
    Particle.function("vib",vibrate);

}


void loop() {

    int amp = amplitude();

 /*
    Serial.print(amp);
    Serial.print("    ");
    Serial.println(Q);
*/

    analogWrite(vibpin,amp);

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

    int analogvalue = analogRead(squeezePin);

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
        y=th+midPower+midPower*sin(2*pi*t/period);
      }
    }

    if (Q<-10000) {
        Q=-10000;
    }
    else if (Q>10000) {
        Q=10000;
    }

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

    int diff = abs(analogvalue-baseline);
    int M=0;
    int B=0;

    // inaction range
    if (diff<inactionThreshold) {
        M = inactionM;
        B = inactionB;
    }
    // pet range
    if (diff>inactionThreshold && diff < petThreshold) {
        M = petM;
        B = petB;
    }
    // squeeze range
    if (diff>squeezeThreshold) {
        M = squeezeM;
        B = squeezeB;
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
