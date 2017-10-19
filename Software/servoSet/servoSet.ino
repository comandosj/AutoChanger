/*************************************************** 

  This sketch is intended to set the precise positions of the servos
  for the Autochanger, and save this configuration in to the eeprom.

 ****************************************************/

#include <Wire.h>
#include <Servo.h>
#include <EEPROM.h>

// The colour changer only uses 4 servos.
byte servos[4][3] = {
  {A3,1,100},
  {A2,1,100},
  {A1,25,145},
  {A0,1,110},
};

unsigned int servoCount[4] = {0,0,0,0};

// don't forget, our servo's start at 0!
byte pattern[4][21] = {
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {3,0,1,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {4,0,1,2,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

Servo myservo[4];

// our servo # counter
uint8_t servonum = 0;

// EEPROM Addresses
const int EEPROMServoCount = 10;
const int EEPROMServo = 50;
const int EEPROMPattern = 100;

void setup() {
  Serial.begin(115200);

  for (int x = 0; x < 4; x++) {
      myservo[x].attach(servos[x][0]);
      myservo[x].write(servos[x][1]);
      Serial.print("Servo ");
      Serial.print(x);
      Serial.print(": Position ");
      Serial.println(servos[x][1]);
  }

  writeEEPROM();

  // this resets the servo counts.
  // Don't use it.
  //EEPROM.put(EEPROMServoCount,servoCount);
  
}

void loop() {
  
}

void writeEEPROM() {

  // Write the servos
  Serial.println("Servo configs");
  for(int i = 0; i< 4; i++) {
    for(int j = 0; j < 3; j++) {
      Serial.print(servos[i][j]);
      Serial.print(":");
    }
    Serial.println();
  }

  EEPROM.put(EEPROMServo,servos);

  //Write the Patterns
  Serial.println("Patterns");
  for(int i = 0; i< 4; i++) {
    for(int j = 0; j < 21; j++) {
      Serial.print(pattern[i][j]);
      Serial.print(":");
    }
    Serial.println();
  }
  EEPROM.put(EEPROMPattern,pattern);
  
}


