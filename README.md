# ESP32-CAM Motion Detection & Notification System

This project implements a motion-activated camera system using an ESP32-CAM module. When motion is detected via a PIR sensor, the ESP32-CAM captures a photo, sends it via email, uploads it to Supabase Storage, and logs the event (with timestamp and image URL) to a Supabase database.

## Features

- **Motion Detection:** Uses a PIR sensor to detect movement.
- **Photo Capture:** Captures an image with the ESP32-CAM when motion is detected.
- **Email Notification:** Sends the captured image as an email attachment to a specified recipient.
- **Supabase Integration:**
  - **Storage:** Uploads the photo to Supabase Storage.
  - **Database Logging:** Logs event details (device, timestamp, image URL) to a Supabase table.
- **Configurable Cooldown:** Prevents multiple rapid triggers via a cooldown period.

## Hardware Requirements

- ESP32-CAM module (AI Thinker recommended)
- PIR Motion Sensor
- LED or Flash (optional, for night mode/photo illumination)
- Jumper wires, breadboard, etc.

## Wiring

| ESP32-CAM Pin | PIR Sensor Pin | Function   |
|---------------|---------------|------------|
| 15            | OUT           | PIR Output |
| 4             | LED/Flash     | Flash      |
| GND           | GND           | Ground     |
| 3.3V          | VCC           | Power      |

> The camera pin configuration is set for AI Thinker module by default.

## Software Requirements

- ESP32 core for Arduino ([installation guide](https://github.com/espressif/arduino-esp32))
- Libraries:
  - `esp_camera.h`
  - `WiFi.h`
  - `ESP_Mail_Client.h`
  - `HTTPClient.h`
  - `time.h`

Install these libraries via the Arduino IDE Library Manager or PlatformIO.

## Configuration

Update the following configuration sections in your code:

```cpp
// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Email SMTP credentials
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define AUTHOR_EMAIL "your_email@gmail.com"
#define AUTHOR_PASSWORD "your_app_password"
#define RECIPIENT_EMAIL "recipient_email@gmail.com"

// Supabase config
#define SUPABASE_URL "https://your-project.supabase.co"
#define SUPABASE_API_KEY "your_anon_key"
#define SUPABASE_TABLE "motion_events"
#define SUPABASE_BUCKET "photos"
```

**Important:**  
- For Gmail, you must use an [App Password](https://support.google.com/accounts/answer/185833?hl=en) (not your main password).
- You can obtain your Supabase API keys and URLs from your Supabase project dashboard.

## How It Works

1. **Startup:** ESP32 connects to WiFi, initializes the camera, and prepares the PIR sensor & flash.
2. **Motion Detection:** Upon motion, the flash turns on, and a photo is taken.
3. **Notification:**
    - The photo is sent via email to the configured recipient.
    - The photo is uploaded to Supabase Storage.
    - An event (timestamp, device, and photo URL) is logged in the Supabase table.
4. **Cooldown:** No new notifications are sent until the cooldown period expires.

## Supabase Table Example (motion_events)

The `motion_events` table should contain at least:
- `id`: integer, primary key
- `device`: text
- `event_time`: timestamp
- `image_url`: text (optional, stores the Supabase public URL)

## Security Notes

- Do **not** commit your credentials (WiFi, email, API keys) to public repositories.
- For production, consider storing secrets securely and not hardcoding them in your source code.

## Example Output

- Serial monitor logs connection status, errors, and actions taken.
- Email received with subject "Motion Detected!" and attached photo.
- Image available in your Supabase Storage "photos" bucket.
- Event logged in Supabase database with timestamp and image URL.

## Troubleshooting

- **Camera Init Failed:** Check camera wiring, use correct board settings.
- **Email Not Sending:** Double-check App Password and SMTP host/port.
- **Supabase Upload Failure:** Ensure API key and bucket name are correct, and your network allows outbound HTTPS.
- **No Motion Detected:** Check PIR sensor wiring and orientation.

## License

MIT License

## Credits

- [ESP Mail Client Library](https://github.com/mobizt/ESP-Mail-Client)
- [Supabase](https://supabase.com/)
- [AI Thinker ESP32-CAM](https://randomnerdtutorials.com/esp32-cam-video-streaming-face-recognition-arduino-ide/)

---

**Feel free to modify, improve, and share this project!**