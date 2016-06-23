/*
Circuit wired like so:
GreenStarboardLED = pin 6 
RedPortLED = pin 7
GreenStarboardSwitch = pin 10 switch wired to ground using internal pullup
RedPortSwitch = pin 11 switch wired to ground using internal pullup
*/

// pins for the switch and LEDs
const int GreenStarboardLED = 6;
const int RedPortLED = 7;
const int GreenStarboardSwitch = 11;
const int RedPortSwitch = 10;
int incomingByte;      // a variable to read incoming serial data into
int greenPressed = 0;
int redPressed = 0;

void setup() {
  // initialize serial communication:
  Serial.begin(9600);
  // initialize the LED pin as an output:
 pinMode(GreenStarboardLED, OUTPUT);
 pinMode(RedPortLED, OUTPUT); 
 pinMode(GreenStarboardSwitch, INPUT_PULLUP);
 pinMode(RedPortSwitch, INPUT_PULLUP);

 digitalWrite(GreenStarboardLED, LOW);
 digitalWrite(RedPortLED, LOW);
 delay(2000);
 digitalWrite(GreenStarboardLED, HIGH);
 digitalWrite(RedPortLED, HIGH);
}

void loop() {
 // read the sensor:
  if (Serial.available() > 0) {
    int inByte = Serial.read();
    // do something different depending on the character received.  
    // The switch statement expects single number values for each case;
    // in this exmaple, though, you're using single quotes to tell
    // the controller to get the ASCII value for the character.  For 
    // example 'a' = 97, 'b' = 98, and so forth:

    switch (inByte) {
    case 'g':    
      digitalWrite(GreenStarboardLED, LOW);
      break;
    case 'r':    
      digitalWrite(RedPortLED, LOW);
      break;
    case 'b':    
      digitalWrite(GreenStarboardLED, HIGH);
      break;
    case 'f':    
      digitalWrite(RedPortLED, HIGH);
      break;
      }
   }
        // read the input pin:
    int buttonState = digitalRead(GreenStarboardSwitch);
    if ( buttonState == 0 ) {
      if ( greenPressed != 1 ) {
        greenPressed = 1;
        Serial.println("[GREEN]");
      } 
    } else {
      greenPressed = 0; 
    }
    buttonState = digitalRead(RedPortSwitch);
    if ( buttonState == 0 ) {
      if ( redPressed != 1 ) {
        redPressed = 1;
        Serial.println("[RED]");
      } 
    } else {
      redPressed = 0; 
    }
    delay(1);        // delay in between reads for stability 
     
  }

