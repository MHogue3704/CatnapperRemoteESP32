// Remote_DeepSleep_OLED.ino
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_now.h>
#include <WiFi.h>
#include "esp_sleep.h"

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

RTC_DATA_ATTR int lastAction = 0;
unsigned long lastActive = 0;
bool sleeping = false;
#define SLEEP_TIMEOUT_MS 15000

uint8_t receiverAddress[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}; // ← Replace with your receiver MAC

typedef struct {
  bool up;
  bool down;
} ChairCommand;

ChairCommand command;

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

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("D&M Specialties");
  display.setCursor(15, 35);
  display.println("Catnapper Remote");
  display.display();
  delay(2000);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  lastActive = millis();
}

void loop() {
  bool up = !digitalRead(BUTTON_UP);
  bool down = !digitalRead(BUTTON_DOWN);
  command.up = up;
  command.down = down;

  if (up || down) {
    lastAction = up ? 1 : 2;
    lastActive = millis();
  }

  esp_now_send(receiverAddress, (uint8_t *)&command, sizeof(command));

  int raw = analogRead(BATTERY_PIN);
  float voltage = (raw / 4095.0) * 3.3 * 2.0;
  int battery = map(voltage * 100, 300, 420, 0, 100);
  battery = constrain(battery, 0, 100);

  bool charging = (digitalRead(CHARGING_PIN) == LOW);

  displayStatus(battery, voltage, charging, up, down);
  delay(200);

  if (millis() - lastActive > SLEEP_TIMEOUT_MS) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0);
    esp_sleep_enable_ext1_wakeup((1ULL << GPIO_NUM_5), ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_deep_sleep_start();
  }
}

