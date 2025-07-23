#include <WiFi.h>
#include <esp_camera.h>

const char* ssid = "vivo 1820"; // Ganti dengan SSID WiFi Anda
const char* password = "87654321"; // Ganti dengan password WiFi Anda

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi kamera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Resolusi
  if (psramFound()) {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Menginisialisasi kamera
  esp_camera_init(&config);

  // Koneksi ke WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  // Ambil gambar
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Kirim gambar ke server Flask
  WiFiClient client;
  if (client.connect(serverUrl, 80)) {
    client.println("POST /upload_image HTTP/1.1");
    client.println("Host: YOUR_SERVER_IP");
    client.println("Content-Type: multipart/form-data; boundary=boundary");
    client.print("Content-Length: ");
    client.println(fb->len + 100); // Estimasi panjang konten
    client.println();

    // Kirim gambar
    client.println("--boundary");
    client.println("Content-Disposition: form-data; name=\"file\"; filename=\"image.jpg\"");
    client.println("Content-Type: image/jpeg");
    client.println();
    client.write(fb->buf, fb->len);
    client.println();
    client.println("--boundary--");

    Serial.println("Image sent to server");
  } else {
    Serial.println("Connection to server failed");
  }

  esp_camera_fb_return(fb);
  delay(10000); // Tunggu 10 detik sebelum mengambil gambar lagi
}