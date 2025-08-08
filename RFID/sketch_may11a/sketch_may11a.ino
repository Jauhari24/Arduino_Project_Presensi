#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// --- Konfigurasi Pin ---
#define RST_PIN  D3    // RST RFID
#define SDA_PIN  D4    // SDA RFID
#define BUZZER_PIN D8  // Buzzer aktif HIGH

// --- Objek RFID ---
MFRC522 mfrc522(SDA_PIN, RST_PIN);

// --- Objek LCD (I2C 0x27, ukuran 16x2) ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- WiFi & MQTT ---
const char* ssid = "vivo 1820";
const char* password = "87654321";
const char* mqtt_server = "7a92772b2a1b442e832825dd8d7aa2b1.s2.eu.hivemq.cloud";
const int mqtt_port = 8883; // port TLS
const char* mqtt_user = "Jauhari";
const char* mqtt_pass = "Salam12345";



// Client secure
WiFiClientSecure espClientSecure;
PubSubClient client(espClientSecure);

// --- Data Kartu Terdaftar ---
const int maxKartu = 10;
String kartuTerdaftar[maxKartu] = {
  "04 3A 8B 22",
  "B3 5C 1A 9F"
};
int jumlahKartu = 2;

// --- Fungsi Koneksi WiFi ---
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Menghubungkan ke ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// --- Reconnect MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_pass)) {
      Serial.println("Terhubung!");
      client.subscribe("alat/register"); // untuk mendaftarkan kartu
      client.subscribe("alat/login");    // untuk login
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi...");
      delay(2000);
    }
  }
}


// --- Tambah Callback ---
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Pesan diterima [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Register kartu baru
  if (String(topic) == "alat/register") {
    if (jumlahKartu < maxKartu) {
      kartuTerdaftar[jumlahKartu] = message;
      jumlahKartu++;
      Serial.println("Kartu baru terdaftar: " + message);
      client.publish("alat/feedback", ("Kartu terdaftar: " + message).c_str());
    } else {
      Serial.println("Daftar kartu penuh!");
      client.publish("alat/feedback", "Daftar kartu penuh!");
    }
  }

  // Login via MQTT (validasi UID dari web/app)
  if (String(topic) == "alat/login") {
    bool cocok = false;
    for (int i = 0; i < jumlahKartu; i++) {
      if (message == kartuTerdaftar[i]) {
        cocok = true;
        break;
      }
    }
    if (cocok) {
      Serial.println("Login sukses untuk UID: " + message);
      client.publish("alat/feedback", ("Login sukses: " + message).c_str());
      tone(BUZZER_PIN, 1000, 200);
    } else {
      Serial.println("Login gagal untuk UID: " + message);
      client.publish("alat/feedback", ("Login gagal: " + message).c_str());
      tone(BUZZER_PIN, 400, 500);
    }
  }
}

void setup() {
  Serial.begin(9600);

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();

  // LCD
  Wire.begin(D2, D1); // SDA, SCL
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Siap");
  lcd.setCursor(0, 1);
  lcd.print("Tempel Kartu");

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // WiFi Secure & MQTT
  setup_wifi();
  espClientSecure.setInsecure(); // set sertifikat CA
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Cek Kartu
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Baca UID
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();

  Serial.println("UID: " + uid);

  // Kirim ke MQTT
  String pesan = "Kartu: " + uid;
  client.publish("esp8266/status", pesan.c_str());

  // Cek apakah UID terdaftar
  bool cocok = false;
  for (int i = 0; i < jumlahKartu; i++) {
    if (uid == kartuTerdaftar[i]) {
      cocok = true;
      break;
    }
  }

  if (cocok) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kartu Cocok");
    lcd.setCursor(0, 1);
    lcd.print(uid);

    tone(BUZZER_PIN, 1000, 200); // bunyi 200 ms
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kartu Tidak");
    lcd.setCursor(0, 1);
    lcd.print("Terdaftar");

    tone(BUZZER_PIN, 400, 500); // bunyi 500 ms
  }

  delay(1000);
}
