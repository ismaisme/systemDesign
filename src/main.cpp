#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// 3 LED, 1 Buz, 1 button
#define redLED 4
#define grnLED 16
#define bluLED 17
#define buzPin 33
#define pushBtn 34

// --- RC522 RFID PINS ---
#define SS_PIN  5
#define RST_PIN 22

MFRC522 rfid(SS_PIN, RST_PIN); // Create MFRC522 instance

// Function Prototype
void blinkRedLed();
void blinkGrnLed();
void displayState(String currState);

void setup() {
  Serial.begin(115200);

  // Initialize SPI bus and RFID sensor
  SPI.begin(); 
  rfid.PCD_Init();

  pinMode(pushBtn, INPUT);

  pinMode(redLED, OUTPUT);
  pinMode(grnLED, OUTPUT);
  pinMode(bluLED, OUTPUT);

	digitalWrite(grnLED, LOW);
  digitalWrite(redLED, LOW);
  digitalWrite(bluLED, LOW);

	Serial.println("\nSetup completed\n\n\n");
}

// Very simple and tidy loop that calls two functions
void loop() {
  static unsigned long mainMillis = millis();

  // countdown for timeout
  static uint8_t countdown;
  // Delay until elevator arrives
  static uint16_t elevatorDelay;

  // Timer for notification process has completed
  static unsigned long beepMillis;

  // Declare the states in meaningful English. Enums start enumerating
  // at zero, incrementing in steps of 1 unless overridden. We use an
  // enum 'class' here for type safety and code readability
  enum class state : uint8_t {
      IDLE,      // defaults to 0
      SCAN,    // defaults to 1
      OPENED,   // defaults to 2
      DOORCLOSE, // defaults to 3
  };

  // Keep track of the current State (it's an elevatorState variable)
  static state currState = state::IDLE;

  // Process according to our State Diagram
  switch (currState) {

      // Initial state (or final returned state)
      case state::IDLE:
          displayState("IDLE state");
          blinkRedLed();
          
          // Someone pushed the button yet?
          if (digitalRead(pushBtn) == LOW) {
              // Set the millis counter for the elevator arrival timer
              mainMillis = millis();
              countdown = 1; 

              // Move to next state
              digitalWrite(redLED, LOW);
              currState = state::SCAN;
              delay(1000);
          }
          break;

      case state::SCAN:
          displayState("SCAN state\nCountdown: ");
          // Light the 'elevator called' LED
          digitalWrite(bluLED, HIGH);
          
          // 1. Check if exactly 1 second (1000ms) has passed
          if (millis() - mainMillis >= 1000) {
              // Reset the timer for the NEXT second
              mainMillis = millis(); 
              
              // Print the countdown text
              Serial.print(".");

              // Increment the counter AFTER printing
              countdown++; 
              
              // 2. If 10 seconds pass (countdown goes from 1 to 11), timeout!
              if (countdown > 10) {
                  Serial.println("\nTimeout! Returning to IDLE.");
                  digitalWrite(bluLED, LOW);
                  currState = state::IDLE;
                  break; // Exit the case immediately
              }
          }
          
          // 2. Check if a new RFID card is present
          if (rfid.PICC_IsNewCardPresent()) {
              // Verify if the UID has been read cleanly
              if (rfid.PICC_ReadCardSerial()) {
                  Serial.println("\n[RFID] Card Detected successfully!");
                  
                  // Optional: Print the UID numbers to Serial Monitor
                  Serial.print("Card UID: ");
                  for (byte i = 0; i < rfid.uid.size; i++) {
                      Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
                      Serial.print(rfid.uid.uidByte[i], HEX);
                  }
                  Serial.println();

                  // Halt PICC to stop reading the same card repeatedly
                  rfid.PICC_HaltA();

                  // Valid card detected! Transition to OPENED state
                  currState = state::OPENED;
              }
          }
          break;


      case state::OPENED:
          displayState("OPENED State");

          // Quick beep to alert user that elevator has arrived
          tone(buzPin, 3000, 1000);

          // Set the timer for the notification
          beepMillis = millis();

          // Move to next state
          currState = state::DOORCLOSE;
          break;

      case state::DOORCLOSE:
          displayState("DOORS CLOSED state");

          // Extinguish the LED
          digitalWrite(bluLED, LOW);
          blinkGrnLed();
          // Time to turn off the beeper yet?
          if (millis() - beepMillis >= 2000) {
              // Turn off beeper
              noTone(buzPin);

              // Move to next state
              digitalWrite(grnLED, LOW);
              currState = state::IDLE;
          }
          break;

      default:
          // Nothing to do here
          Serial.println("'Default' Switch Case reached - Error");
  }
}

// RED led blink
void blinkRedLed() {
	// This unsigned long integer declaration is only 
	// actioned ONCE by the compiler before moving the
	// variable outside of the loop
  static unsigned long redMillis = millis();

	// If enough time has passed (500mS) we toggle the flash
  if (millis() - redMillis > 500) {
		// Set the red LED to whatever it is NOT now
    digitalWrite(redLED, !digitalRead(redLED));
		
		// We must reset the local millis variable
    redMillis = millis();
  }
}

// GREEN led blink - same as above, just runs faster
void blinkGrnLed() {
  static unsigned long grnMillis = millis();

  if (millis() - grnMillis > 100) {
    digitalWrite(grnLED, !digitalRead(grnLED));
    grnMillis = millis();
  }
}

// Helper routine to track state machine progress
void displayState(String currState) {
    static String prevState = "";

    if (currState != prevState) {
        Serial.println(); Serial.println(currState);
        prevState = currState;
    }
}