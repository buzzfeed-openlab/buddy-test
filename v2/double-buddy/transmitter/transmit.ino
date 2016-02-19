// Firmware for the transmitting buddy
// when he is squeezed, he will purr to represent the degree of squeeze
// He will also transmit this purr to the receiver
// the receiver will purr but provide no feedback to the transmitting buddy

// declare as transmitter

// connect to receiver

// send serial command to receiver every loop

//#define VIBPIN D0
//#define LED D7
//#define RECEIVER


char msg; // the message you plan to send to the other module
char feedback; // any messages you get back from the other module

void setup() {
//  pinMode(VIBPIN, OUTPUT);
//  pinMode(LED, OUTPUT);

  Serial1.begin(38400);
  Serial.begin(38400);

  Serial.println("Hello.");

  // now write the commands to make the microcontroller connect to the transmitting module

  Serial1.print("AT\r\n");
  Serial1.write("AT\r\n");

}

void loop() {
  Serial1.write("AT\r\n");
  Serial1.print("AT\r\n");
  while(Serial1.available()) { // read all available bytes
    Serial.write((char)Serial1.read()); // write does no fancy stuff like print might
    Serial.println("."); // to 'see' how many bytes we get at a time
  }
}
