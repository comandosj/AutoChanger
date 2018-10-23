/*************************************************** 
 *  
 *  
 *  EEPROM Mapping
 *  

 ****************************************************/

#include <Wire.h>
#include <Servo.h>
#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>
#include <EEPROM.h>

#define DEBUG 1 // Do we want debug output on serial.
#define BUZZER 1 // Is the buzzer on or off.

// Set our version number.  Don't forget to update when featureset changes
#define VERSION "AutoChanger V.1.5"


Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, 4, NEO_GRB + NEO_KHZ800);

// My Neopixels need a high and a low value.
byte color[2][3]; 

// The colour changer only uses 4 servos.
byte servos[4][3];/* = {
  {A3,1,100},
  {A2,1,100},
  {A1,25,145},
  {A0,1,110},
};*/

Servo myservo[4];
unsigned long servoTimeout;

// our servo # counter
uint8_t servonum = 1;
uint8_t maxServo = 4;

// We track how many movements the servo has made.
// because, why not?! :)
unsigned int servoCount[4];
unsigned long EEPromTimeStamp;
unsigned int EEPromUpdateTime = 60000;


// arm Trigger

const int armPin = 13;
unsigned int buttonState;             // the current reading from the input pin
unsigned int lastButtonState = HIGH;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 15;    // the debounce time; increase if the output flickers
byte armCounter = 0;  // We ignore every second pass.

// input buttons
const int buttonPins[4] = {10,9,8,7};

const int pgrmPin = 6;

// Debouncing
Bounce debouncer[4] = Bounce();
Bounce pgrmDebouncer = Bounce();
unsigned long buttonPressTimeStamp;
boolean triggerPgrm = false;
boolean pgrmMode = false;
unsigned long pgrmTimeStamp;

// Buzzer
const int buzzerPin = 3;

// Array of ints to store our pattern.
byte currentPattern = 0;
byte pattern[4][21];/* = {
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {3,0,1,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {4,0,1,2,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};
*/

// EEPROM Addresses
const int EEPROMServoCount = 10;
const int EEPROMServo = 50;
const int EEPROMPattern = 100;

void setup() {
  Serial.begin(115200);

  // First, build information
  Serial.println(F(VERSION));
  Serial.print(F("Build Date: "));
  Serial.println(F(__DATE__ " " __TIME__));
  Serial.print(F("Free Ram: "));
  Serial.println(freeRam());

  // get our previous settings.
  readEEPROM();

  pinMode(armPin, INPUT);

  for (int x = 0; x < 4; x++) {
    pinMode(buttonPins[x], INPUT);
    debouncer[x].attach(buttonPins[x]);
    debouncer[x].interval(5);
  }

  pinMode(pgrmPin, INPUT);
  pgrmDebouncer.attach(pgrmPin);
  pgrmDebouncer.interval(5);

  pinMode(buzzerPin, OUTPUT);

  // reset all the servo positions.
  for (int x = 0; x < maxServo; x++) {
    if(pattern[currentPattern][0] && pattern[currentPattern][1] == x)
      moveServo(x,2);
    else
      moveServo(x,1);
  }
  
  pixels.begin();
  //pixels.setPixelColor(0, pixels.Color(20,0,0));
  byte temp[3] = {20,0,0};
  setPixels(temp);
  pixels.show();

  // if the program pin is high, trigger a simulator mode.
  if(!digitalRead(pgrmPin)) {
    TestRun();
  }
}

void readEEPROM() {
  byte offset = 0;
  //Read our values from EEPROM

  // Read what the saved patterns.
  EEPROM.get(EEPROMPattern, pattern);
  if(DEBUG) {
    Serial.println(F("Patterns: "));
    for(int x = 0; x < 4; x++) {
      Serial.print(F("  "));
      for(int y = 0; y < 21; y++) {
        Serial.print(pattern[x][y]);
        Serial.print(F(":"));
      }
      Serial.println();
    }
  }

  // Read the Servo config.
  EEPROM.get(EEPROMServo, servos);
  if(DEBUG) {
    Serial.println(F("Servo Config: "));
    for(int x = 0; x < 4; x++) {
      Serial.print(F("  "));
      for(int y = 0; y < 3; y++) {
        Serial.print(servos[x][y]);
        Serial.print(F(":"));
      }
      Serial.println();
    }
  }
  

  // Read our current servo iterations.
  EEPROM.get(EEPROMServoCount,servoCount);

  // Show us how many actions each servo has made.
  // This happens even if not in debug mode
  Serial.println(F("Servo movement counts: "));
  for(int i = 0; i < 4; i++) {
    Serial.print(F("  Servo "));
    Serial.print(i);
    Serial.print(F(": "));
    Serial.println(servoCount[i]);
  }
}

void writeEEPROM() {
  // Every 60 seconds we update the eeprom.
  // This is safe and should only write if the value has changed.
  if ( millis() > EEPromTimeStamp+EEPromUpdateTime) {
    Serial.println(F("Updating EEPROM."));

    // update the patterns

    // no need to update the servo config, that can't be set in this sketch.

    // update the servo counts.
    EEPROM.put(EEPROMServoCount,servoCount);
    
    EEPromTimeStamp = millis();
  }
}

// we don't really need to hold the servos
// this function turns the servo on, moves it, then switches it off.
// x is the servo number.
// y is the position to move to in the "servos" array
void moveServo(int x, int y) {
  
  myservo[x].attach(servos[x][0]);

  if(myservo[x].read() != servos[x][y]) {
  // only update servoCount if the read position is different to where we're trying to get to.
    servoCount[x]++;
    
    myservo[x].write(servos[x][y]);
    if(DEBUG) {
      Serial.print(F("Moving Servo: "));
      Serial.print(x);
      if(y == 1)
        Serial.println(F(" down."));
      else
        Serial.println(F(" up."));
    }
  }
  
  servoTimeout = millis();
}

// after a given time, detach all servos.
void detachServo() {
  if(servoTimeout + 400 < millis()) {
    for (int x = 0; x < maxServo; x++) {
      myservo[x].detach();
    }
  }
}

void switchRods() {

  moveServo(pattern[currentPattern][servonum],1);

  servonum ++;
  
  if (servonum > pattern[currentPattern][0]) {
    if(DEBUG)
      Serial.println(F("Reset to start"));
    servonum = 1;
  }

  moveServo(pattern[currentPattern][servonum],2);

}

void programMode() {
  // The theory is this is triggered after a 2-3 second hold.
  // then the other buttons can be pressed to program the unit.
  

}

void loop() {

  // Update our buttons.
  for (int x = 0; x < 4; x++) {
    debouncer[x].update();
  }
  pgrmDebouncer.update();

  // Check if we need to update the arm
  // We only need to do this if our pattern is non-zero
  // If the pattern is 0, it's a manual pattern
  if(pattern[currentPattern][0])
    checkArm();

  // run our button combo section
  checkButtons();

  if(armCounter)
    pixels.setPixelColor(0, pixels.Color(color[1][0],color[1][1],color[1][2]));
  else
    pixels.setPixelColor(0, pixels.Color(color[0][0],color[0][1],color[0][2]));

  detachServo();
  pixels.show();

  writeEEPROM();
}

// look for our button combinations.
void checkButtons() {
  boolean pgrm = (!pgrmDebouncer.read());
  boolean one = (!debouncer[0].read());
  boolean two = (!debouncer[1].read());
  boolean three = (!debouncer[2].read());
  boolean four = (!debouncer[3].read());

  // There are several possible actions if number buttons are pressed
  // 1. If pgrm is also pressed, change pattern.
  // 2. If this is the active servo, cycle to the next in the sequence.
  // 3. If this is not the active servo, temporary swap
  // 4. If we have no pattern, swap servos.
  // 5. If pgrm is held down, go in to program mode.
  // We assume only one number button at a time.

  if(one) {
    byte temp[3] = {20,0,0};
    buttonActions(0, pgrm, temp);
  } else if (two) {
    byte temp[3] = {0,20,0};
    buttonActions(1, pgrm, temp);
  } else if (three) {
    byte temp[3] = {0,0,20};
    buttonActions(2, pgrm, temp);
  } else if (four) {
    byte temp[3] = {10,0,10};
    buttonActions(3, pgrm, temp);
  } else if (pgrm) {
    // first, reset the arm counter
    // It won't really matter if this happens before we head in to programming mode.
    if(armCounter != 0) {
      armCounter = 0;
      if(DEBUG)
        Serial.println(F("Arm Counter reset."));
    }

    // right, determine if the program button has been held down long enough to go in to programming mode.
    // start programming mode
    // Flash the LED's or something.
    if (pgrmTimeStamp == 0) {
      pgrmTimeStamp = millis();
    }
    else if (pgrmTimeStamp + 2000 >= millis()) {
      pgrmMode = true;
    }
    
  } else {
    pgrmTimeStamp = 0;
  }
}

void buttonActions(byte button, boolean pgrm, byte colours[3]) {
  // we repeat these actions a lot
  if(pgrm) {
      // ok, this only needs to happen if our current servo isn't already the one we want to switch to
      if(pattern[currentPattern][servonum] != pattern[button][1]) {
        moveServo(pattern[currentPattern][servonum],1); // put our current servo down.
      } else if (!pattern[currentPattern][0] && pattern[button][1] != servonum) {
        // special edge case, if our previous pattern was empty, put all the servos down.
        // oh, and we re-used servonum to act as our current servo, rather than a position in the
        // pattern array.
        for (int x = 0; x < 4; x++) {
            moveServo(x,1);
        }
      }
      
      currentPattern = button;
      servonum = 1; // first in the pattern.
      // only need to do stuff if the pattern is active
      if( pattern[currentPattern][0]) {
        moveServo(pattern[currentPattern][servonum],2);
      }
      
      setPixels(colours);
    } else if (!pattern[currentPattern][0]) {
      // No pattern, flick down all the other servos and flick this one up.
      for (int x = 0; x < 4; x++) {
        if(x == button)
          moveServo(x,2);
        else
          moveServo(x,1);
      }
      servonum = button;
    } else if (pattern[currentPattern][servonum] == button) {
      // This is the active servo.
      switchRods();
    } else {
      // this is a temporary switch.
      // broken lets disable it
      /*
      moveServo(pattern[currentPattern][servonum],1); // put our current servo down.
      moveServo(0,2); // put this servo up.
      while(debouncer[0].read()) {
        // pretty much do nothing
      }
      moveServo(0,1); // put this servo down.
      moveServo(pattern[currentPattern][servonum],2); // move old servo up.
      */
    }
}

void setPixels(byte colours[3]) {
  byte tempColor1[] = {colours[0], colours[1], colours[2]};
  memcpy(color[0],tempColor1,3);
  byte tempColor2[] = {colours[0]*2, colours[1]*2, colours[2]*2};
  memcpy(color[1],tempColor2,3);
}


// Just pull this out in to it's own function to make this easier to read.
void checkArm() {
  int reading = digitalRead(armPin);

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        if(armCounter == 0){
          switchRods();
          armCounter = 1;

          // Turn the buzzer on for 1 second
          if(BUZZER)
            tone(buzzerPin, "c", 1000);
        }
        else
          armCounter = 0;
      }
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
}

void TestRun() {
  // Trigger a testrun if the program button is held down during start.

  pixels.setPixelColor(0, pixels.Color(20,20,20));
  pixels.show();
  delay(100);

  Serial.println(F("Entering Test Mode."));
  Serial.println(F("Executing 100 cycles of the servos."));
  
  for (int y = 0; y < 100; y++) {
    for (int x = 0; x < maxServo; x++) {
      if(x == 0) {
        moveServo(maxServo-1,1);
        pixels.setPixelColor(0, pixels.Color(20,0,0));
      } else
        moveServo(x-1,1);

      if( x == 1)
        pixels.setPixelColor(0, pixels.Color(0,20,0));
      else if( x == 2)
        pixels.setPixelColor(0, pixels.Color(0,0,20));
      else if( x == 3)
        pixels.setPixelColor(0, pixels.Color(10,0,10));
        
      moveServo(x,2);

      
      pixels.show();
      delay(400);
      writeEEPROM(); // update the eeprom if needed.
    }
  }
}

int freeRam(void)
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

