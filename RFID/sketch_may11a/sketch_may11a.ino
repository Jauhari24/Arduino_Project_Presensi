#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// --- Konfigurasi Pin ---
#define RST_PIN  D3
#define SDA_PIN  D4
#define BUZZER_PIN D8

MFRC522 mfrc522(SDA_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Variabel state ---
enum Mode { LOGIN, REGISTER };
Mode mode = LOGIN;
unsigned long registerStartTime = 0;
bool waitingRegister = false;

String lastUID = "";
unsigned long lastScanTime = 0;
unsigned long cooldown = 3000; // 2 detik jeda antar-scan

// --- MQTT Topics ---
const char* t_login_status    = "alat/login_status";
const char* t_register_status = "alat/register_status";
const char* t_message         = "alat/message";
const char* t_rfid_login      = "data/rfid/login";
const char* t_rfid_register   = "data/rfid/register";

// --- WiFi & MQTT ---
const char* ssid = "Konsol";
const char* password = "20011116..";
const char* mqtt_server = "5ca84e9de5c4426bb1862989f9503bbd.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "Jauhari";
const char* mqtt_pass = "Salam12345";

WiFiClientSecure espClientSecure;
PubSubClient client(espClientSecure);

// --- Fungsi Buzzer ---
void beep(bool success) {
  if (success) tone(BUZZER_PIN, 1000, 200);
  else tone(BUZZER_PIN, 400, 500);
}

// --- LCD ---
void printLCD(const char* line1, const char* line2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(line1);
  lcd.setCursor(0, 1); lcd.print(line2);
}

// --- Koneksi WiFi ---
void setup_wifi() {
  Serial.print("Menghubungkan ke ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
}

// --- Reconnect MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_pass)) {
      Serial.println("Terhubung!");
      client.subscribe(t_login_status);
      client.subscribe(t_register_status);
      client.subscribe(t_message);
      client.subscribe(t_rfid_login);
      client.subscribe(t_rfid_register);
    } else {
      Serial.print("Gagal, rc="); Serial.println(client.state());
      delay(2000);
    }
  }
}

// --- MQTT Callback ---
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  String topicStr = String(topic);

  Serial.printf("MQTT [%s]: %s\n", topic, msg.c_str());

  if (topicStr == t_message) {
    if (msg == "login_success") {
      printLCD("Login", "Berhasil");
      beep(true);
    }
    else if (msg == "login_failed") {
      printLCD("Login", "Gagal");
      beep(false);
    }
    else if (msg == "register_success") {
      printLCD("Register", "Berhasil");
      beep(true);
      delay(3000); // tunggu 3 detik
      mode = LOGIN;
      waitingRegister = false;
      printLCD("Mode Login", "Tempel Kartu");
    }
    else if (msg == "register_failed") {
      printLCD("Register", "Gagal");
      beep(false);
    }
    else {
      // Pesan umum lainnya
      printLCD(msg.c_str());
      beep(true);
    }
  }

   if (topicStr == t_register_status && msg == "1") {
    mode = REGISTER;
    waitingRegister = true;
    registerStartTime = millis();
    printLCD("Mode Register", "Tempel Kartu");
    beep(true);
  }
  else {
    mode = LOGIN;
    waitingRegister = false;
    printLCD("Mode Login", "Tempel Kartu");
    beep(true);
  }

}

void setup() {
  Serial.begin(9600);

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();

  // LCD
  Wire.begin(D2, D1);
  lcd.init();
  lcd.backlight();
  printLCD("Sistem Siap", "Tempel Kartu");

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // WiFi & MQTT
  setup_wifi();
  espClientSecure.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  // Timeout register mode (15 detik)
  if (mode == REGISTER && waitingRegister && millis() - registerStartTime > 15000) {
    mode = LOGIN;
    waitingRegister = false;
    printLCD("Mode Login", "Tempel Kartu");
    beep(false);
  }

  // Cek kartu baru
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Ambil UID kartu
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();

  unsigned long now = millis();

  // Cegah spam: cek UID terakhir dan cooldown
  if (uid == lastUID && (now - lastScanTime < cooldown)) {
    Serial.println("Scan diabaikan (cooldown)");
    return;
  }

  lastUID = uid;
  lastScanTime = now;

  // Mode login
  if (mode == LOGIN) {
    Serial.println("Login UID: " + uid);
    client.publish(t_rfid_login, uid.c_str());
    printLCD("Login...", uid.c_str());
    beep(true);
  }
  // Mode register
  else if (mode == REGISTER && waitingRegister) {
    Serial.println("Register UID: " + uid);
    client.publish(t_rfid_register, uid.c_str());
    waitingRegister = false;
  }
}
