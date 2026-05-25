/**
 * ============================================================
 *  Dual nRF24L01 Jammer – RF‑Link Detection + Library Jamming
 *  ESP32 + 2x nRF24L01 on HSPI and VSPI
 * ============================================================
 *
 *  Detection: manual SPI (carrier TX on HSPI, RX on VSPI)
 *  Jamming:   RF24 library startConstCarrier() sweep
 *  Sweep range: channels 0‑125 (2.400‑2.525 GHz)
 *  Logging: prints every 1000 sweeps the current channels
 *
 *  ⚠️  LEGAL WARNING:
 *  Unauthorised jamming is illegal. Operate ONLY inside a
 *  sealed Faraday enclosure. For educational/research use.
 * ============================================================
 */

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <WiFi.h>           // for WiFi.mode(WIFI_OFF)
#include "esp_bt.h"         // for btStop()

// ---------- Pin definitions ----------
#define HSPI_SCK   14
#define HSPI_MISO  12
#define HSPI_MOSI  13
#define HSPI_CS    15    // CSN
#define HSPI_CE    16    // CE

#define VSPI_SCK   18
#define VSPI_MISO  19
#define VSPI_MOSI  23
#define VSPI_CS    21    // CSN
#define VSPI_CE    22    // CE

// ---------- Manual SPI buses (detection only) ----------
SPIClass hspi_man(HSPI);
SPIClass vspi_man(VSPI);

// ---------- RF24 library objects (jamming) ----------
SPIClass hspi(HSPI);
SPIClass vspi(VSPI);
RF24 radioH(16, 15);   // HSPI module
RF24 radioV(22, 21);   // VSPI module

// ---------- Manual SPI helpers ----------
void writeRegH(uint8_t reg, uint8_t value) {
  hspi_man.beginTransaction(SPISettings(10000000, SPI_MSBFIRST, SPI_MODE0));
  digitalWrite(HSPI_CS, LOW);
  hspi_man.transfer(0x20 | (reg & 0x1F));
  hspi_man.transfer(value);
  digitalWrite(HSPI_CS, HIGH);
  hspi_man.endTransaction();
}

uint8_t readRegH(uint8_t reg) {
  uint8_t val;
  hspi_man.beginTransaction(SPISettings(10000000, SPI_MSBFIRST, SPI_MODE0));
  digitalWrite(HSPI_CS, LOW);
  hspi_man.transfer(0x00 | (reg & 0x1F));
  val = hspi_man.transfer(0xFF);
  digitalWrite(HSPI_CS, HIGH);
  hspi_man.endTransaction();
  return val;
}

void writeRegV(uint8_t reg, uint8_t value) {
  vspi_man.beginTransaction(SPISettings(10000000, SPI_MSBFIRST, SPI_MODE0));
  digitalWrite(VSPI_CS, LOW);
  vspi_man.transfer(0x20 | (reg & 0x1F));
  vspi_man.transfer(value);
  digitalWrite(VSPI_CS, HIGH);
  vspi_man.endTransaction();
}

uint8_t readRegV(uint8_t reg) {
  uint8_t val;
  vspi_man.beginTransaction(SPISettings(10000000, SPI_MSBFIRST, SPI_MODE0));
  digitalWrite(VSPI_CS, LOW);
  vspi_man.transfer(0x00 | (reg & 0x1F));
  val = vspi_man.transfer(0xFF);
  digitalWrite(VSPI_CS, HIGH);
  vspi_man.endTransaction();
  return val;
}

// ---------- Banner ----------
void printBanner() {
  Serial.println();
  Serial.println(F("####################################################################################################"));
  Serial.println(F("███████ ███████ ██████  ██████  ██████        ██████  ██      ██    ██ ███████      ██  █████  ███    ███ ███    ███ ███████ ██████ "));
  Serial.println(F("██      ██      ██   ██      ██      ██       ██   ██ ██      ██    ██ ██           ██ ██   ██ ████  ████ ████  ████ ██      ██   ██"));
  Serial.println(F("█████   ███████ ██████   █████   █████  █████ ██████  ██      ██    ██ █████        ██ ███████ ██ ████ ██ ██ ████ ██ █████   ██████ "));
  Serial.println(F("██           ██ ██           ██ ██            ██   ██ ██      ██    ██ ██      ██   ██ ██   ██ ██  ██  ██ ██  ██  ██ ██      ██   ██"));
  Serial.println(F("███████ ███████ ██      ██████  ███████       ██████  ███████  ██████  ███████  █████  ██   ██ ██      ██ ██      ██ ███████ ██   ██"));
  Serial.println(F("####################################################################################################"));
  Serial.println(F("                                          Firmware : Combo-Channel-Select (BT-BLE-WiFi-RC)                                          "));
  Serial.println(F("                       ██████╗ ██╗   ██╗    ███████╗███╗   ███╗███████╗███╗   ██╗███████╗████████╗ █████╗                      "));
  Serial.println(F("                       ██╔══██╗╚██╗ ██╔╝    ██╔════╝████╗ ████║██╔════╝████╗  ██║██╔════╝╚══██╔══╝██╔══██╗                     "));
  Serial.println(F("                       ██████╔╝ ╚████╔╝     █████╗  ██╔████╔██║█████╗  ██╔██╗ ██║███████╗   ██║   ███████║                     "));
  Serial.println(F("                       ██╔══██╗  ╚██╔╝      ██╔══╝  ██║╚██╔╝██║██╔══╝  ██║╚██╗██║╚════██║   ██║   ██╔══██║                     "));
  Serial.println(F("                       ██████╔╝   ██║       ███████╗██║ ╚═╝ ██║███████╗██║ ╚████║███████║   ██║   ██║  ██║                     "));
  Serial.println(F("                       ╚═════╝    ╚═╝       ╚══════╝╚═╝     ╚═╝╚══════╝╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚═╝  ╚═╝                     "));
  Serial.println(F("####################################################################################################"));
  Serial.println(F("                                                    !Educational purposes only!                                                     "));
  Serial.println(F("                                         https://github.com/EmenstaNougat/ESP32-BlueJammer                                          "));
  Serial.println(F("                                               I'm not responsible for your actions!                                                "));
  Serial.println(F("####################################################################################################"));
}

// ---------- Jamming sweep with logging ----------
void jamForever() {
  const uint8_t firstChannel = 0;
  const uint8_t lastChannel  = 125;
  const int dwellMs = 1;

  uint8_t chEven = firstChannel;
  uint8_t chOdd  = firstChannel + 1;
  unsigned long sweepCount = 0;

  radioH.startConstCarrier(RF24_PA_MAX, chEven);
  radioV.startConstCarrier(RF24_PA_MAX, chOdd);

  while (true) {
    radioH.powerDown();
    radioV.powerDown();

    chEven = (chEven + 2 > lastChannel) ? firstChannel : chEven + 2;
    chOdd  = (chOdd  + 2 > lastChannel) ? firstChannel + 1 : chOdd + 2;

    radioH.startConstCarrier(RF24_PA_MAX, chEven);
    radioV.startConstCarrier(RF24_PA_MAX, chOdd);

    sweepCount++;
    if (sweepCount % 1000 == 0) {
      Serial.printf("Sweep #%lu – channels: %d & %d\n", sweepCount, chEven, chOdd);
    }

    delay(dwellMs);
  }
}

// ---------- Manual RF‑link detection ----------
void configureManualTX() {
  digitalWrite(HSPI_CE, LOW);
  digitalWrite(HSPI_CS, HIGH);
  writeRegH(0x00, 0x00);                 // Power down
  delayMicroseconds(150);
  writeRegH(0x05, 40);                   // Channel 40
  writeRegH(0x06, 0b10001110);           // CONT_WAVE=1, 0 dBm
  writeRegH(0x07, 0x70);                 // Clear status
  writeRegH(0x00, 0b00001010);           // PWR_UP, TX mode
  delay(2);
  digitalWrite(HSPI_CE, HIGH);           // Start carrier
}

void configureManualRX() {
  digitalWrite(VSPI_CE, LOW);
  digitalWrite(VSPI_CS, HIGH);
  writeRegV(0x00, 0x00);                 // Power down
  delayMicroseconds(150);
  writeRegV(0x05, 40);                   // Same channel
  writeRegV(0x00, 0b00001011);           // PWR_UP, PRIM_RX=1
  delay(2);
  digitalWrite(VSPI_CE, HIGH);           // Start listening
}

bool manualCarrierDetected() {
  uint8_t rpd = readRegV(0x09) & 0x01;
  return (rpd == 1);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(2000);

  printBanner();

  pinMode(HSPI_CE, OUTPUT);
  pinMode(HSPI_CS, OUTPUT);
  digitalWrite(HSPI_CE, LOW);
  digitalWrite(HSPI_CS, HIGH);

  pinMode(VSPI_CE, OUTPUT);
  pinMode(VSPI_CS, OUTPUT);
  digitalWrite(VSPI_CE, LOW);
  digitalWrite(VSPI_CS, HIGH);

  hspi_man.begin(HSPI_SCK, HSPI_MISO, HSPI_MOSI, HSPI_CS);
  vspi_man.begin(VSPI_SCK, VSPI_MISO, VSPI_MOSI, VSPI_CS);

  Serial.println(F("\nState 0: Waiting for RF link between HSPI (TX) and VSPI (RX)..."));
  Serial.println(F("Power on both nRF24L01 modules. Keep them close together."));

  bool linkEstablished = false;

  while (!linkEstablished) {
    configureManualTX();
    configureManualRX();
    delay(50);

    if (manualCarrierDetected()) {
      linkEstablished = true;
      Serial.println(F("\nRF link confirmed! Both modules are alive."));
    } else {
      digitalWrite(HSPI_CE, LOW);
      digitalWrite(VSPI_CE, LOW);
      Serial.print(F("."));
      delay(2000);
    }
  }

  // Shut down radios and end manual SPI buses completely
  digitalWrite(HSPI_CE, LOW);
  digitalWrite(VSPI_CE, LOW);
  writeRegH(0x00, 0x00);
  writeRegV(0x00, 0x00);
  delay(100);

  hspi_man.end();   // release HSPI peripheral
  vspi_man.end();   // release VSPI peripheral
  delay(100);

  // Now initialize library SPI buses (fresh)
  hspi.begin();   // uses HSPI default pins
  vspi.begin();   // uses VSPI default pins

  if (!radioH.begin(&hspi)) {
    Serial.println(F("FATAL: HSPI radio init failed (library)"));
    while (1);
  }
  radioH.setPALevel(RF24_PA_MAX);
  radioH.setAutoAck(false);
  radioH.stopListening();

  if (!radioV.begin(&vspi)) {
    Serial.println(F("FATAL: VSPI radio init failed (library)"));
    while (1);
  }
  radioV.setPALevel(RF24_PA_MAX);
  radioV.setAutoAck(false);
  radioV.stopListening();

  WiFi.mode(WIFI_OFF);
  btStop();

  Serial.println(F("Initialisation complete. Starting full‑band sweep (channels 0‑125)..."));
  Serial.println(F("JAMMING ACTIVE. Only a hardware reset will stop it."));
  delay(1000);

  jamForever();
}

void loop() {}