#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Ganti dengan WiFi kamu
const char* ssid = "vivo 1820";
const char* password = "87654321";

// Ganti dengan HiveMQ info kamu
const char* mqtt_server = "7a92772b2a1b442e832825dd8d7aa2b1.s2.eu.hivemq.cloud";  // broker dari HiveMQ
const int mqtt_port = 8883;                             // gunakan 1883 jika tidak pakai TLS
const char* mqtt_user = "Jauhari";
const char* mqtt_pass = "Salam12345";

WiFiClientSecure espClient;  // Untuk koneksi TLS (gunakan WiFiClient biasa kalau pakai port 1883)
PubSubClient client(espClient);

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

  Serial.println("");
  Serial.println("WiFi terkoneksi!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Ulangi sampai konek
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    // Buat client ID unik
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Terhubung ke MQTT!");
      // Bisa subscribe di sini kalau mau
      // client.subscribe("topik/tes");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();

  // Hapus jika kamu menggunakan port 1883 (tanpa TLS)
  espClient.setInsecure();  // ⛔️ Hanya untuk testing TLS tanpa sertifikat

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Kirim data setiap 5 detik
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 5000) {
    lastSend = millis();
    String pesan = "Halo dari ESP8266!";
    client.publish("esp8266/tes", pesan.c_str());
    Serial.println("Pesan dikirim ke MQTT: " + pesan);
  }
}

