#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_now.h>
#include <WiFi.h>
#include "esp_sleep.h"

// OLED configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin Definitions
#define BUTTON_UP     4
#define BUTTON_DOWN   5
#define BATTERY_PIN   0
#define CHARGING_PIN  10
#define SDA_PIN       7
#define SCL_PIN       6

// Timeout before going to deep sleep (milliseconds)
#define SLEEP_TIMEOUT_MS 15000

// Persisted between deep-sleep cycles
RTC_DATA_ATTR unsigned long lastActive = 0;

// Replace with your receiver's MAC address
uint8_t receiverAddress[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};

typedef struct {
  bool up;
  bool down;
} ChairCommand;

ChairCommand command;

// Draw status on the OLED
void displayStatus(int battery, float voltage, bool charging, bool up, bool down) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Battery: ");
  display.print(battery);
  display.print("%");

  display.setCursor(0, 10);
  display.print("Voltage: ");
  display.print(voltage, 2);
  display.print(" V");

  if (charging) {
    display.setCursor(0, 20);
    display.print("Charging...");
  }

  display.setCursor(0, 40);
  display.print("UP: ");
  display.print(up ? "ON" : "OFF");

  display.setCursor(64, 40);
  display.print("DOWN: ");
  display.print(down ? "ON" : "OFF");

  display.display();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Wire.begin(SDA_PIN, SCL_PIN);

  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(CHARGING_PIN, INPUT);

  // Initialize OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("D&M Specialties");
  display.setCursor(15, 35);
  display.println("Catnapper Remote");
  display.display();
  delay(2000);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  }

  lastActive = millis();
}

void loop() {
  static bool lastUp = false;
  static bool lastDown = false;
  static unsigned long lastDisp = 0;

  bool up = (digitalRead(BUTTON_UP) == LOW);
  bool down = (digitalRead(BUTTON_DOWN) == LOW);

  // Send command only when state changes
  if (up != lastUp || down != lastDown) {
    command.up = up;
    command.down = down;
    esp_now_send(receiverAddress, (uint8_t*)&command, sizeof(command));
    lastUp = up;
    lastDown = down;
    lastActive = millis();
  }

  // Update display every second
  if (millis() - lastDisp > 1000) {
    lastDisp = millis();
    int raw = analogRead(BATTERY_PIN);
    float voltage = (raw / 4095.0) * 3.3 * 2.0;
    int battery = map(int(voltage * 100), 300, 420, 0, 100);
    battery = constrain(battery, 0, 100);
    bool charging = (digitalRead(CHARGING_PIN) == LOW);
    displayStatus(battery, voltage, charging, up, down);
  }

  // Deep sleep after timeout
  if (millis() - lastActive > SLEEP_TIMEOUT_MS) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
    // Wake on either button press
    uint64_t wakeMask = (1ULL << BUTTON_UP) | (1ULL << BUTTON_DOWN);
    esp_sleep_enable_ext1_wakeup(wakeMask, ESP_EXT1_WAKEUP_ANY_LOW);
    esp_deep_sleep_start();
  }
}
