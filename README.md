# TODO

## Device used
- RGB LED
  - 4, 16, 17
 
- buzzer
  - 33

- button
  - 34

- NFC Reader (RFID RC522)
  - 22, 23, 19, 18, 5

- TFT Display (2.8 TFT SPI 240x320 v1.2)
  - T_IRQ	GPIO 36
  - T_OUT	GPIO 39
  - T_DIN	GPIO 32
  - T_CS	GPIO 33
  - T_CLK	GPIO 25
  - SDO(MISO)	GPIO 12
  - LED	GPIO 21
  - SCK	GPIO 14
  - SDI(MOSI)	GPIO 13
  - D/C	GPIO 2
  - RESET	EN/RESET
  - CS	GPIO 15
  - GND	GND
  - VCC	5V (or 3.3V)*
- Update device used here
- Update pinout to esp32 here

## Propose new pin allocation
TFT display
- SCK 18
- SDA 23
- RST 13
- D/C 14
- CS  16

- T-CLK 18
- T-DI  23
- T-DO  19
- T-CS  17

NFC
- SS 5
- RST 22
- CLK 18
- MOSI 23
- MISO 19

Load cell
- DT 34
- CLK 4

Magnetic lock
- 33

## Implemtation using both SPI (VSPI and HSPI)

TFT Display (VSPI)
- TFT MISO 19
- TFT MOSI 23 (Touch only)
- SCLK 18
- CS 16
- DC 17
- RST 4
- T_CS 21
- BL 3v3

NFC RC522 (HSPI)
- RFID_RST_PIN    27
- RFID_SS_PIN     26
- HSPI_CLK        14
- HSPI_MISO       12
- HSPI_MOSI       13
