#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// === PIN SETUP ===
// LCD I2C
#define I2C_SDA D2  // GPIO4
#define I2C_SCL D1  // GPIO5

// RFID RC522 (SPI)
#define RST_PIN D3  // GPIO0
#define SS_PIN  D4  // GPIO2

// Buzzer
#define BUZZER_PIN D8  // GPIO15

// Inisialisasi RFID
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Inisialisasi LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Ganti 0x27 ke 0x3F jika tidak muncul teks

// Data kartu
const int maxKartu = 10;
String kartuTerdaftar[maxKartu] = {
  "", "", "AB CD EF 12", "34 56 78 9A"  // Contoh UID terdaftar
};
int jumlahKartu = 4;  // Jumlah UID terisi awal

bool modeDaftar = false;

void setup() {
  // Serial Monitor
  Serial.begin(9600);

  // LCD
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" Tempel Kartu ");

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();
  delay(500);
  Serial.println("Sistem Siap. Ketik 'DAFTAR' untuk daftar kartu.");
}

void loop() {
  // Cek input dari serial
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();
    if (input == "DAFTAR") {
      Serial.println("Masuk mode daftar, tempelkan kartu...");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Mode Daftar ");
      lcd.setCursor(0, 1);
      lcd.print("Tempel Kartu");
      modeDaftar = true;
    }
  }

  // Cek kartu
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Ambil UID
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();

  Serial.println("UID: " + uid);

  if (modeDaftar) {
    if (jumlahKartu < maxKartu) {
      kartuTerdaftar[jumlahKartu] = uid;
      jumlahKartu++;
      Serial.println("✅ Kartu berhasil didaftarkan.");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Kartu Didaftar");
      lcd.setCursor(0, 1);
      lcd.print(uid);
      bunyiBuzzer(1000);  // Lama bunyi lebih panjang
    } else {
      Serial.println("⚠️ Penyimpanan penuh!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Penuh!");
      bunyiBuzzer(100);  // Bunyi pendek
    }
    modeDaftar = false;
    delay(1500);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Tempel Kartu ");
    return;
  }

  // Cek UID terdaftar
  bool cocok = false;
  for (int i = 2; i < jumlahKartu; i++) {
    if (uid == kartuTerdaftar[i]) {
      cocok = true;
      break;
    }
  }

  if (cocok) {
    Serial.println("✅ Kartu cocok!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Akses Diterima ");
    lcd.setCursor(0, 1);
    lcd.print(uid);
    bunyiBuzzer(500);
  } else {
    Serial.println("❌ Kartu tidak cocok.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Akses Ditolak ");
    lcd.setCursor(0, 1);
    lcd.print(uid);
    bunyiBuzzer(100);
  }

  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Tempel Kartu ");
}

// Fungsi buzzer
void bunyiBuzzer(int durasiMs) {
  tone(BUZZER_PIN, 1000);  // 1 kHz
  delay(durasiMs);
  noTone(BUZZER_PIN);
}
