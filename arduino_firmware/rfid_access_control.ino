#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

// === Pinout ===
// MFRC522
constexpr uint8_t PIN_SS = 10;  // SDA
constexpr uint8_t PIN_RST = 9;
// LCD 16x2 (4 bits)
constexpr uint8_t PIN_LCD_RS = 7;
constexpr uint8_t PIN_LCD_EN = 6;
constexpr uint8_t PIN_LCD_D4 = 5;
constexpr uint8_t PIN_LCD_D5 = 4;
constexpr uint8_t PIN_LCD_D6 = 3;
constexpr uint8_t PIN_LCD_D7 = 2;
// Buzzer
constexpr uint8_t PIN_BUZZER = 8;

MFRC522 rfid(PIN_SS, PIN_RST);
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

String readUID();
String readLineFromSerial(uint16_t timeoutMs = 4000);
void showAccessMessage(bool granted, const String &name = "");
void beep(bool granted);

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(4000);

  SPI.begin();
  rfid.PCD_Init();

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Badge prêt...");

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = readUID();
  if (uid.isEmpty()) {
    showAccessMessage(false);
    return;
  }

  lcd.clear();
  lcd.print("Envoi ID...");
  Serial.print("ID:");
  Serial.println(uid);

  String response = readLineFromSerial();
  response.trim();

  if (response.startsWith("OK:")) {
    int firstColon = response.indexOf(':', 3);
    String nom = response.substring(3, firstColon > 0 ? firstColon : response.length());
    String prenom = firstColon > 0 ? response.substring(firstColon + 1) : "";
    String fullName = prenom + " " + nom;
    showAccessMessage(true, fullName);
  } else {
    showAccessMessage(false);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(1500);
}

String readUID() {
  // Convert the UID bytes to a single 64-bit decimal number so the value
  // exactly matches the numeric ID_EMPLOYE stored in the database.
  uint64_t value = 0;
  for (byte i = 0; i < rfid.uid.size; i++) {
    value = (value << 8) | rfid.uid.uidByte[i];
  }
  return String(value);
}

String readLineFromSerial(uint16_t timeoutMs) {
  unsigned long start = millis();
  while ((millis() - start) < timeoutMs) {
    if (Serial.available()) {
      return Serial.readStringUntil('\n');
    }
  }
  return "";
}

void showAccessMessage(bool granted, const String &name) {
  lcd.clear();
  if (granted) {
    lcd.print("Bienvenue, ");
    lcd.setCursor(0, 1);
    lcd.print(name.substring(0, 16));
  } else {
    lcd.print("Acces refuse");
    lcd.setCursor(0, 1);
    lcd.print("Reessayer");
  }
  beep(granted);
}

void beep(bool granted) {
  if (granted) {
    tone(PIN_BUZZER, 2000, 120);
    delay(150);
    tone(PIN_BUZZER, 2500, 120);
  } else {
    tone(PIN_BUZZER, 400, 400);
  }
  delay(100);
  noTone(PIN_BUZZER);
}
