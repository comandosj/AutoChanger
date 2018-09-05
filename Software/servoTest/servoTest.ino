
#include <Bounce2.h>


const int armPin = 13;



unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 1;    // the debounce time; increase if the output flickers


void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  //pinMode(armPin, INPUT);

  
}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.println(digitalRead(armPin));

  
}


