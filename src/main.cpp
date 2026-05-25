#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

TFT_eSPI tft = TFT_eSPI();

// --- Dedicated MFRC522 Pins (HSPI Bus) ---
#define RFID_RST_PIN    27          
#define RFID_SS_PIN     26   
#define HSPI_CLK        14
#define HSPI_MISO       12
#define HSPI_MOSI       13

// Instantiate the dedicated HSPI hardware class
SPIClass hspiSPI(HSPI);

// Create the modular components required by MFRC522v2
MFRC522DriverPinSimple ss_pin(RFID_SS_PIN);
// Pass both the CS pin driver and our dedicated HSPI bus instance to the SPI driver
MFRC522DriverSPI driver{ss_pin, hspiSPI}; 
MFRC522 mfrc522{driver};

// Declare the states in meaningful English using enum class
enum class state : uint8_t {
  IDLE,      
  SCAN,    
  OPENED,   
  DOORCLOSE, 
};

// Keep track of the current State
static state currState = state::IDLE;
static bool stateChanged = true; // Prevents the screen from flickering by drawing only once

void drawDefaultScreen(const char* statusText, bool invert) {
  tft.invertDisplay(invert);
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_GREEN);
  tft.setCursor(0, 4, 4);

  tft.setTextColor(TFT_WHITE);
  tft.print(" Status:"); 
  tft.println(statusText);
  tft.println(" White text");
  
  tft.setTextColor(TFT_RED);
  tft.println(" Red text");
  
  tft.setTextColor(TFT_GREEN);
  tft.println(" Green text");
  
  tft.setTextColor(TFT_BLUE);
  tft.println(" Blue text");
}

void handleStateMachine(void);

void setup() {
  Serial.begin(115200);

  // 1. Initialize your screen on the primary default bus
  tft.init();
  tft.setRotation(0);
  drawDefaultScreen("Ready to Scan", false);

  // --- Touch Calibration ---
  // These are standard baseline calibration coordinates for TFT_eSPI
  uint16_t calData[5] = { 200, 3700, 240, 3600, 0 };
  tft.setTouch(calData);

  // 2. Initialize the dedicated HSPI bus interface using your clean pins
  hspiSPI.begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI, RFID_SS_PIN);

  // 3. Boot up the RFID reader using the v2 driver initialization layout
  mfrc522.PCD_Init(); 
  delay(4);

  Serial.println(F("Setup Complete"));
}

void loop() {
  handleStateMachine();  // Task 2: Process elevator inputs, timings, and RFID
}

void handleStateMachine() {
  static unsigned long mainMillis = millis();
  static uint8_t countdown = 0;
  static unsigned long beepMillis = 0;

  uint16_t t_x = 0, t_y = 0; // Variables to store touch locations

  switch (currState) {
    case state::IDLE:
      if (stateChanged) {
          drawDefaultScreen("IDLE", false);
          stateChanged = false;
      }

      // Check if screen is pressed
      if (tft.getTouch(&t_x, &t_y)) {
          currState = state::SCAN;
          stateChanged = true;
          countdown = 0;
          mainMillis = millis();
          delay(300); // Simple debounce to prevent double-triggering
      }
      break;

    case state::SCAN:
      if (stateChanged) {
            drawDefaultScreen("SCAN", true);
            stateChanged = false;
        }
      
      if (millis() - mainMillis >= 1000) {
          mainMillis = millis(); 
          Serial.print(".");
          countdown++; 
          
        if (countdown > 10) {
            Serial.println("\nTimeout! Returning to IDLE.");
            currState = state::IDLE;
            break; 
        }
      }
      
      if (mfrc522.PICC_IsNewCardPresent()) {
        if (mfrc522.PICC_ReadCardSerial()) {
          Serial.println("\n[RFID] Card Detected successfully!");
          
          Serial.print("Card UID: ");
          for (byte i = 0; i < mfrc522.uid.size; i++) {
              Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
              Serial.print(mfrc522.uid.uidByte[i], HEX);
          }
          Serial.println();

          mfrc522.PICC_HaltA();
          currState = state::OPENED;
        }
      }
      break;

    case state::OPENED:
        drawDefaultScreen("OPENED", true);
        // digitalWrite(relay, HIGH);
        // tone(buzPin, 3000, 1000);
        // beepMillis = millis();
        delay(1000);
        currState = state::DOORCLOSE;
        break;

    case state::DOORCLOSE:
      if (stateChanged) {
          drawDefaultScreen("DOORCLOSE", false); // Fixed copy-paste status text string
          stateChanged = false;
      }
    
      if (millis() - beepMillis >= 2000) {
          // noTone(buzPin);
          // digitalWrite(grnLED, LOW);
          currState = state::IDLE;
          stateChanged = true;
      }
      // digitalWrite(relay, LOW);
      break;

    default:
        Serial.println("'Default' Switch Case reached - Error");
  }
}
