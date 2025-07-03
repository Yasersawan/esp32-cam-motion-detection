#include "esp_camera.h"
#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <HTTPClient.h>
#include <time.h> // For timestamp

// WiFi credentials
const char* ssid = "Your_SSID";
const char* password = "Your_Password";

// Email credentials/config
#define SMTP_HOST "smtp.gmail.com"  // Use your SMTP server, e.g., Gmail
#define SMTP_PORT 465 // Use 465 for SSL, 587 for TLS
#define AUTHOR_EMAIL "****@gmail.com"
#define AUTHOR_PASSWORD "App Password" // Use an App Password if 2FA is enabled
#define RECIPIENT_EMAIL "******@mail.com"

// Supabase config (replace with your values)
#define SUPABASE_URL "https://your-project.supabase.co"
#define SUPABASE_API_KEY "Your_Supabase_API_Key" // Use anon key
#define SUPABASE_TABLE "motion_events"
#define SUPABASE_BUCKET "photos"

// PIR and FLASH GPIOs
#define PIR_PIN 15
#define FLASH_PIN 4

SMTPSession smtp;
bool sentRecently = false;
unsigned long lastSendTime = 0;
const unsigned long sendCooldown = 10000; // 10 seconds

// Camera config (AI Thinker module)
camera_config_t camera_config = {
  .pin_pwdn = 32,
  .pin_reset = -1,
  .pin_xclk = 0,
  .pin_sscb_sda = 26,
  .pin_sscb_scl = 27,
  .pin_d7 = 35,
  .pin_d6 = 34,
  .pin_d5 = 39,
  .pin_d4 = 36,
  .pin_d3 = 21,
  .pin_d2 = 19,
  .pin_d1 = 18,
  .pin_d0 = 5,
  .pin_vsync = 25,
  .pin_href = 23,
  .pin_pclk = 22,
  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_JPEG,
  .frame_size = FRAMESIZE_VGA,
  .jpeg_quality = 12,
  .fb_count = 1
};


void sendEmail(uint8_t *imageBuf, size_t imageLen) {
  SMTP_Message message;
  message.sender.name = "ESP32-CAM";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "Motion Detected!";
  message.addRecipient("Owner", RECIPIENT_EMAIL);
  message.text.content = "Motion detected! Photo attached.";
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  SMTP_Attachment att;
  att.descr.filename = "photo.jpg";
  att.blob.data = imageBuf;
  att.blob.size = imageLen;
  att.descr.mime = "image/jpeg";
  message.addAttachment(att);

  smtp.debug(1);
  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  if (!smtp.connect(&session)) {
    Serial.println("SMTP connection failed!");
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.print("Error sending Email: ");
    Serial.println(smtp.errorReason());
  } else {
    Serial.println("Email sent successfully!");
  }
  smtp.closeSession();
}

// Helper to get ISO8601 timestamp
String getISOTime() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buf);
}

// Upload photo to Supabase Storage
String uploadPhotoToSupabase(const uint8_t* imageBuf, size_t imageLen) {
  HTTPClient http;
  String fileName = "esp32-photo-";
  fileName.concat(String(millis()));
  fileName.concat(".jpg");

  String uploadURL = SUPABASE_URL;
  uploadURL.concat("/storage/v1/object/");
  uploadURL.concat(SUPABASE_BUCKET);
  uploadURL.concat("/");
  uploadURL.concat(fileName);

  http.begin(uploadURL);
  http.addHeader("apikey", SUPABASE_API_KEY);
  http.addHeader("Authorization", "Bearer " SUPABASE_API_KEY);
  http.addHeader("Content-Type", "image/jpeg");

  int httpResponseCode = http.PUT((uint8_t*)imageBuf, imageLen);

  if (httpResponseCode > 0) {
    Serial.print("Upload response: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());
    // Return the storage path or public URL
    String publicURL = SUPABASE_URL;
    publicURL.concat("/storage/v1/object/public/");
    publicURL.concat(SUPABASE_BUCKET);
    publicURL.concat("/");
    publicURL.concat(fileName);
    return publicURL;
  } else {
    Serial.print("Upload failed, error: ");
    Serial.println(http.errorToString(httpResponseCode));
    return "";
  }
  http.end();
}
// Log a motion event to Supabase (with optional image URL)
void logMotionEventToSupabase(const String& imageUrl) {
  HTTPClient http;

  // Build the endpoint
  String url;
  url.reserve(256);
  url = SUPABASE_URL;
  url.concat("/rest/v1/");
  url.concat(SUPABASE_TABLE);
  http.begin(url);

  http.addHeader("apikey", SUPABASE_API_KEY);
  http.addHeader("Authorization", "Bearer " SUPABASE_API_KEY);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=representation");

  // Build the JSON body with ISO timestamp
  String nowISO = getISOTime();
  String body;
  body.reserve(256);
  body = "{\"device\":\"esp32-cam\",\"event_time\":\"";
  body.concat(nowISO);
  body.concat("\"");
  if (imageUrl.length() > 0) {
    body.concat(",\"image_url\":\"");
    body.concat(imageUrl);
    body.concat("\"");
  }
  body.concat("}");

  int httpResponseCode = http.POST(body);

  if (httpResponseCode > 0) {
    Serial.print("Supabase response code: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());
  } else {
    Serial.print("Supabase POST failed, error: ");
    Serial.println(http.errorToString(httpResponseCode));
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(FLASH_PIN, OUTPUT);
  digitalWrite(FLASH_PIN, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // Get NTP time for timestamps
  configTime(0, 0, "pool.ntp.org");

  if (esp_camera_init(&camera_config) != ESP_OK) {
    Serial.println("Camera init failed");
    while (1);
  }
}

void loop() {
  int pirState = digitalRead(PIR_PIN);
  unsigned long now = millis();

  if (pirState == HIGH && !sentRecently && (now - lastSendTime > sendCooldown)) {
    sentRecently = true;
    lastSendTime = now;
    Serial.println("Motion detected!");

    digitalWrite(FLASH_PIN, HIGH); // Turn flash ON
    delay(100);
    camera_fb_t *fb = esp_camera_fb_get();
    delay(1000); // Let LED light up

    digitalWrite(FLASH_PIN, LOW);  // Turn flash OFF

    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    sendEmail(fb->buf, fb->len);

    // Upload to Supabase Storage
    String imageUrl = uploadPhotoToSupabase(fb->buf, fb->len);

    // Log to Supabase with image URL
    logMotionEventToSupabase(imageUrl);

    esp_camera_fb_return(fb);
  }
  if (pirState == LOW) {
    sentRecently = false;
  }
  delay(100);
}