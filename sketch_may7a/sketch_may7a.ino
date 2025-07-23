#include "esp_camera.h"
#include <WiFi.h>

// Ganti dengan nama dan password WiFi Anda
const char* ssid = "Konsol";
const char* password = "20011116..";

// Definisikan nilai untuk LEDC_CHANNEL dan LEDC_TIMER
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER LEDC_TIMER_0

void setup() {
  Serial.begin(115200);
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL;
  config.ledc_timer = LEDC_TIMER;
  config.pin_d0 = 5; // D0 pin
  config.pin_d1 = 18; // D1 pin
  config.pin_d2 = 19; // D2 pin
  config.pin_d3 = 21; // D3 pin
  config.pin_d4 = 36; // D4 pin
  config.pin_d5 = 39; // D5 pin
  config.pin_d6 = 34; // D6 pin
  config.pin_d7 = 35; // D7 pin
  config.pin_xclk = 0; // XCLK pin
  config.pin_pclk = 22; // PCLK pin
  config.pin_vsync = 25; // VSYNC pin
  config.pin_href = 23; // HREF pin
  config.pin_sscb_sda = 26; // SDA pin
  config.pin_sscb_scl = 27; // SCL pin
  config.pin_pwdn = 32; // PWDN pin
  config.pin_reset = -1; // Reset pin
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Resolusi kamera
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12; // 0-63, semakin rendah semakin bagus
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Inisialisasi kamera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera gagal diinisialisasi: %s\n", esp_err_to_name(err));
    return;
  }

  // Menghubungkan ke WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Menghubungkan ke WiFi...");
  }
  Serial.println("Terhubung ke WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Mengambil gambar
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Gagal mengambil gambar");
    return;
  }

  // Mengirim gambar melalui serial
  Serial.write(fb->buf, fb->len);
  Serial.println("Gambar diambil dan dikirim melalui serial");

  // Mengembalikan buffer
  esp_camera_fb_return(fb);
  
  delay(5000); // Ambil gambar setiap 5 detik
}
