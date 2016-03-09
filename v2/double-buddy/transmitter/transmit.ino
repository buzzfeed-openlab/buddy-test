// Adapted from BDub's example in his RF24 Library for Spark Core
// https://github.com/technobly/SparkCore-RF24
// Original example Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>
// distributed under GNU General Public License version 2 as published by the Free Software Foundation.


#include <application.h>

#include "nRF24L01.h"
#include "SparkCore-RF24.h"

// Don't need wifi for this demo
SYSTEM_MODE(SEMI_AUTOMATIC);

// Pins for vibration output and fabric input (transmitter needs both)
#define VIBPIN D0
#define SQUEEZEPIN A0
#define POWERPIN D7

int analogvalue;  // the actual value of squeeze coming from SQUEEZEPIN
int vibMin=50;    // minimum vibration needed for the vibration to be felt
int vibMax=200;   // the maximum vibration produced
int yMin=250;    // the minimum input of the conductive yarn (value when not compressed)
int yMax=600;    // the maximum input of the conductive yarn (value at its most compressed)


/*

Hardware configuration is:

-----------
photon - RF
-----------
GND - GND
3V3 - 3V3
D6  - CE
A2  - CSN
A3  - SCK
A5  - MOSI
A6  - MISO
-----------

vibration motor: D0 and GND
conductive yarn: A0 and 3V3
10K resistor: A0 and GND

  NOTE: Also place a 10-100uF cap across the power inputs of
        the NRF24L01+.  I/O o fthe NRF24 is 5V tolerant, but
        do NOT connect more than 3.3V to pin 1!!!
 */

RF24 radio(D6,A2);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// Two roles: transmit and receive
typedef enum { role_transmit = 1, role_receive } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Transmitter", "Receiver"};

// The role of the current running sketch
role_e role = role_transmit; // Start as a Transmitter

void setup(void)
{

  // Set pin modes
  pinMode(VIBPIN,OUTPUT);

  Particle.function("set",setBounds);
  // Setup and configure rf radio
  radio.begin();

  if ( role == role_transmit )
  {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);
  }
  else
  {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
  }

  // Start listening
  radio.startListening();

  // Dump the configuration of the rf unit for debugging
  radio.printDetails();

}

void loop(void)
{

  if (role == role_transmit) {
    transmitData();
  }

  if ( role == role_receive ) {
    receiveData();
  }

  // To change roles
  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    if ( c == 'T' && role == role_receive )
    {
      SERIAL("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK\n\r");

      // Become the primary transmitter (ping out)
      role = role_transmit;
      radio.openWritingPipe(pipes[0]);
      radio.openReadingPipe(1,pipes[1]);
      radio.stopListening();
    }
    else if ( c == 'R' && role == role_transmit )
    {
      SERIAL("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK\n\r");

      // Become the primary receiver (pong back)
      role = role_receive;
      radio.openWritingPipe(pipes[1]);
      radio.openReadingPipe(1,pipes[0]);
      radio.startListening();
    }
    else if ( c == 'P' )
    {
      SERIAL("*** PRINTING DETAILS\n\r");
      radio.printDetails();
    }
  }
}

void transmitData() {
  int analogvalue = restrict(analogRead(SQUEEZEPIN),yMin,yMax);
  int viblevel = scale(analogvalue,yMin,yMax,vibMin,vibMax);
  analogWrite(VIBPIN,viblevel);

  // Switch to a Receiver before each transmission,
  // or this will only transmit once.
  radio.startListening();

  // Re-open the pipes for Tx'ing
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);

  // First, stop listening so we can talk.
  radio.stopListening();

  // Take the time, and send it.  This will block until complete
  SERIAL("Now sending %i...",viblevel);
  bool ok = radio.write( &viblevel, sizeof(int) );
  if (ok)
    SERIAL("ok...\n\r");
  else
    SERIAL(" failed.\n\r");

  // Try again 100 ms later
  delay(100);
}

void receiveData() {
  // if there is data ready
  if ( radio.available() )
  {
    // Dump the payloads until we've gotten everything
    int viblevel;
    bool done = false;
    while (!done)
    {
      // Fetch the payload, and see if this was the last one.
      done = radio.read( &viblevel, sizeof(int) );

      // Spew it
      //printf("Got payload %lu...\n\r",got_time);
      Serial.print("Got payload "); Serial.println(viblevel);

      // use this value
      analogWrite(VIBPIN,viblevel);

      // Delay just a little bit to let the other unit
      // make the transition to receiver
      delay(20);
    }

    // Switch to a transmitter after each received payload
    // or this will only receive once
    radio.stopListening();

    // Re-open the pipes for Rx'ing
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);

    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }
}

int scale(int x, int xmin, int xmax, int min, int max) {
  int scaled = min+max*(x-xmin)/(xmax-xmin);
  return scaled;
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

int setBounds(String command) {
  char inputStr[64];
  command.toCharArray(inputStr,64);
  char *p = strtok(inputStr,",");
  char *type = p;
  p = strtok(NULL,",");
  int min = atoi(p);
  p = strtok(NULL,",");
  int max = atoi(p);
  if (type=="vib") {
    vibMax=max;
    vibMin=min;
    return 1;
  }
  else if (type=="yarn" || type=="y") {
    yMin=min;
    yMax=max;
    return 1;
  }
  else {
    return 0;
  }
}
