#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/* ===== PINS ===== */
#define SS_PIN     10
#define RST_PIN     9
#define BUZZER_PIN  7

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* ===== ANTI DOUBLE SCAN ===== */
String lastUID = "";
unsigned long lastScanTime = 0;
const unsigned long scanDelay = 1500; // ms

static void showReady()
{
  lcd.clear();
  lcd.print("Scan your card");
}

static void buzz(unsigned long durationMs)
{
  digitalWrite(BUZZER_PIN, LOW);
  delay(durationMs);
  digitalWrite(BUZZER_PIN, HIGH);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    /* wait for serial */
  }

  SPI.begin();
  rfid.PCD_Init();

  lcd.init();
  lcd.backlight();
  showReady();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);

  Serial.println("ARDUINO READY");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial())  return;

  /* ===== BUILD UID (HEX, NO SPACES) ===== */
  String uid;
  uid.reserve(2 * rfid.uid.size);
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += '0';
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  /* ===== PREVENT DUPLICATE READS ===== */
  if (uid == lastUID && millis() - lastScanTime < scanDelay) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  lastUID = uid;
  lastScanTime = millis();

  /* ===== SEND TO QT IN EXPECTED FORMAT ===== */
  // Only "ID:<UID>" then newline. No extra text keeps Qt parser happy.
  Serial.print("ID:");
  Serial.println(uid);

  /* ===== WAIT FOR QT RESPONSE ===== */
  unsigned long start = millis();
  while (!Serial.available()) {
    if (millis() - start > 3000) {
      lcd.clear();
      lcd.print("No Qt Response");
      delay(1500);
      showReady();
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return;
    }
  }

  String response = Serial.readStringUntil('\n');
  response.trim();

  /* ===== HANDLE QT RESPONSE ===== */
  if (response == "NOK") {
    lcd.clear();
    lcd.print("Access Denied");
    buzz(300);
    delay(1500);
    showReady();
  }
  else if (response.startsWith("OK:")) {
    int p1 = response.indexOf(':');
    int p2 = response.lastIndexOf(':');

    String nom = response.substring(p1 + 1, p2);
    String prenom = response.substring(p2 + 1);

    lcd.clear();
    lcd.print("Welcome");
    lcd.setCursor(0, 1);
    lcd.print(nom + " " + prenom);

    buzz(800);
    delay(2000);
    showReady();
  } else {
    // Unknown reply -> show and continue
    lcd.clear();
    lcd.print("Qt reply?");
    lcd.setCursor(0, 1);
    lcd.print(response);
    delay(1500);
    showReady();
  }

  delay(500);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
