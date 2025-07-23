#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN D1
#define SDA_PIN D2

MFRC522 mfrc522(SDA_PIN, RST_PIN);

// Maksimum kartu yang bisa didaftarkan (bisa disesuaikan)
const int maxKartu = 10;
String kartuTerdaftar[maxKartu];
int jumlahKartu = 0;

bool modeDaftar = false;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Sistem Siap. Ketik 'DAFTAR' untuk mendaftarkan kartu baru.");
}

void loop() {
  // Cek apakah ada input dari serial
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();
    if (input == "DAFTAR") {
      Serial.println("Masuk ke mode pendaftaran. Tempelkan kartu sekarang...");
      modeDaftar = true;
    }
  }

  // Jika tidak ada kartu baru, keluar dari loop
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Dapatkan UID
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();

  // Mode daftar
  if (modeDaftar) {
    if (jumlahKartu < maxKartu) {
      kartuTerdaftar[jumlahKartu] = uid;
      jumlahKartu++;
      Serial.println("Kartu berhasil didaftarkan dengan UID: " + uid);
    } else {
      Serial.println("Penyimpanan kartu penuh!");
    }
    modeDaftar = false;
    delay(1000);
    return;
  }

  // Mode cek kartu
  Serial.println("UID tag : " + uid);
  bool cocok = false;
  for (int i = 0; i < jumlahKartu; i++) {
    if (uid == kartuTerdaftar[i]) {
      cocok = true;
      break;
    }
  }

  if (cocok) {
    Serial.println("Pesan : Kartu cocok");
  } else {
    Serial.println("Pesan : Kartu TIDAK cocok");
  }

  delay(1000);
}
