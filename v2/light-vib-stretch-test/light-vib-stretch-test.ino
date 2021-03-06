// test for both light and vibration
// in this case vibration will just respond to light stretch (petting) while light will be an independent variable
// light should in this case get brighter when Buddy "sees" less (aka less light coming in through photoresistors)
// color changes based on squeeze


#include "math.h"
#include "neopixel.h"

#define PIXEL_PIN D1
#define PIXEL_COUNT 1
#define PIXEL_TYPE WS2812B


SYSTEM_MODE(SEMI_AUTOMATIC);


Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// values for LED
int squeezeMax = 500;
int squeezeMin;
// recalibrated values
int redCal;
int grnCal;
int bluCal;

// eyesight values
int left;
int right;
int avg;

int vibpin=D0;
int powerPin=D7;
int squeezePin=A0;
int eyeLeft=A1;
int eyeRight=A2;

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

    strip.begin();
    strip.show(); // Initialize all pixels to 'off'

    digitalWrite(powerPin,HIGH);
    squeezeMin=analogRead(A0);

    Particle.function("analogread",tinkerAnalogRead);
    Particle.function("vib",vibrate);
    Particle.function("see",eyesight);

}


void loop() {

    // do vibration
    int amp = amplitude();
    analogWrite(vibpin,amp);

    // do light
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

    eyesight("avg");

    Serial.print(left);
    Serial.print("     ");
    Serial.println(right);

    // translate that into the light pattern
    strip.setPixelColor(0, strip.Color(redCal,grnCal,bluCal));
    strip.show();
//    colorAll(strip.Color(redCal,grnCal,bluCal),50);


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
