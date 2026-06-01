#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>
#include "HX711.h"

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

// --- Peripherals Pins ---
#define LOADCELL_DT    33
#define LOADCELL_CLK   32
#define MAG_LOCK_PIN   25

#define MAG_LOCK_PIN   25

HX711 scale;

// variable 
double initial_weight = 0.0;
double current_weight = 0.0;
double change_weight  = 0.0;
double total_price = 0.0;
const double PRICE_PER_GRAM = 0.05; 

// Declare the states in meaningful English using enum class
enum class state : uint8_t {
  STATE_IDLE,      
  STATE_OPEN,    
  STATE_LOAD_DETECTING,   
  STATE_CLOSE, 
};

// Keep track of the current State
static state currState = state::STATE_IDLE;
static bool stateChanged = true; // Prevents the screen from flickering by drawing only once

void drawDefaultScreen(const char* statusText, bool invert) {
  tft.invertDisplay(invert);
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_GREEN);
  tft.setCursor(0, 4, 4);

  tft.setTextColor(TFT_WHITE);
  tft.print(" Status:"); 
  tft.println(statusText);
}

void handleStateMachine(void);

void setup() {
  Serial.begin(115200);

  // 1. Initialize your screen on the primary default bus
  tft.init();
  tft.setRotation(0);
  drawDefaultScreen("Ready to STATE_OPEN", false);

  // --- Touch Calibration ---
  // These are standard baseline calibration coordinates for TFT_eSPI
  uint16_t calData[5] = { 200, 3700, 240, 3600, 0 };
  tft.setTouch(calData);

  // 2. Initialize the dedicated HSPI bus interface using your clean pins
  hspiSPI.begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI, RFID_SS_PIN);

  // 3. Boot up the RFID reader using the v2 driver initialization layout
  mfrc522.PCD_Init(); 
  delay(4);

  pinMode(MAG_LOCK_PIN, OUTPUT);
  digitalWrite(MAG_LOCK_PIN, LOW); // Enforce structural locking state

  scale.begin(LOADCELL_DT, LOADCELL_CLK);
  scale.set_offset(-445749); 
  scale.set_scale(386.286530);
  delay(200);
  scale.tare();  

  Serial.println(F("Setup Complete"));
}

void loop() {
  handleStateMachine();  // Task 2: Process elevator inputs, timings, and RFID

  tft.setCursor(5, 280, 4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); 
  tft.print(" Weight: ");

  // 4. Print the actual weight
  tft.setCursor(100, 280, 4);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); 
  tft.print(scale.get_units(1));
  tft.print("g      ");

  tft.setCursor(5, 200, 4);
  tft.print(" Initial: ");
  tft.print(initial_weight);
  tft.print("g      ");

  tft.setCursor(5, 230, 4);
  tft.print(" change: ");
  tft.print(change_weight);
  tft.print("g      ");
}

void handleStateMachine() {
  static unsigned long mainMillis = millis();
  static uint8_t countdown = 0;
  static unsigned long beepMillis = 0;


  uint16_t t_x = 0, t_y = 0; // Variables to store touch locations

  switch (currState) {
    case state::STATE_IDLE:
      if (stateChanged) {
        drawDefaultScreen("STATE_IDLE\nWaiting for card...", false);
        digitalWrite(MAG_LOCK_PIN, LOW);
        stateChanged = false;
      }
      if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        Serial.print("NFC Card Detected! UID:");
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
          Serial.print(mfrc522.uid.uidByte[i], HEX);
        }
        Serial.println();
        mfrc522.PICC_HaltA(); 
        
        currState = state::STATE_OPEN;
        stateChanged = true;  
      }

      break;

    case state::STATE_OPEN:
      if (stateChanged) {
        drawDefaultScreen("STATE_OPEN\nTouch if food taken", false);
        digitalWrite(MAG_LOCK_PIN, HIGH);
        initial_weight = scale.get_units(10);
        if (initial_weight < 0.0) initial_weight = 0.0;
        stateChanged = false;
      }

      
      if (tft.getTouch(&t_x, &t_y)) {
        currState = state::STATE_LOAD_DETECTING;
        stateChanged = true;
        delay(300); // Simple debounce to prevent double-triggering
      }
      break;

    case state::STATE_LOAD_DETECTING:
      if (stateChanged) {
        digitalWrite(MAG_LOCK_PIN, LOW);
        // drawDefaultScreen("STATE_LOAD_DETECTING\nSee if any change...", false);
        current_weight = scale.get_units(10);
        change_weight = current_weight - initial_weight;
        
        total_price = change_weight * PRICE_PER_GRAM;
        delay(1000);

        currState = state::STATE_CLOSE;
        stateChanged = true;
      }

      break;

    case state::STATE_CLOSE:
      if (stateChanged) {
        // drawDefaultScreen("STATE_CLOSE", false); // Fixed copy-paste status text string
        stateChanged = false;
      
        tft.fillScreen(TFT_BLACK);
        tft.drawRect(0, 0, tft.width(), tft.height(), TFT_RED); // Red frame for locked status
        
        tft.setCursor(10, 20, 4);
        tft.setTextColor(TFT_WHITE);
        tft.println("   RECEIPT");
        
        tft.setCursor(10, 70, 4);
        tft.print(" Taken: ");
        tft.print(change_weight, 1);
        tft.println(" g");
        
        tft.setCursor(10, 120, 4);
        tft.print(" Price: $");
        tft.println(total_price, 2);

        tft.setCursor(10, 180, 2);
        tft.setTextColor(TFT_GREEN);
        tft.println(" Securely Locked.");
        tft.println(" Resetting in 5 seconds...");

        // Capture time to start our 5-second countdown window
        mainMillis = millis();
        stateChanged = false;
      }
    
      // 5. Wait non-blockingly for 5 seconds before going back to looking for a card
      if (millis() - mainMillis >= 5000) {
        currState = state::STATE_IDLE;
        stateChanged = true;
      }
    
      break;

    default:
      Serial.println("'Default' Switch Case reached - Error");
  }
}