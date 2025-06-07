#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Pin Definitions for Chair Motor Control
#define RELAY_UP    2  // Controls motor direction: UP
#define RELAY_DOWN  3  // Controls motor direction: DOWN

// Structure matching the remote's payload
typedef struct {
  bool up;
  bool down;
} ChairCommand;

// Callback: called when ESP-NOW data is received
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (len < sizeof(ChairCommand)) return;  // Invalid packet
  ChairCommand cmd;
  memcpy(&cmd, data, sizeof(cmd));

  // Drive the relays according to the command
  digitalWrite(RELAY_UP,   cmd.up   ? HIGH : LOW);
  digitalWrite(RELAY_DOWN, cmd.down ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  // Disable Wi-Fi power saving for lowest latency
  esp_wifi_set_ps(WIFI_PS_NONE);

  // Initialize relay pins
  pinMode(RELAY_UP, OUTPUT);
  pinMode(RELAY_DOWN, OUTPUT);
  digitalWrite(RELAY_UP, LOW);
  digitalWrite(RELAY_DOWN, LOW);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) { delay(1000); }
  }

  // Register the receive callback
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Catnapper Receiver Ready");
}

void loop() {
  // No polling needed; all work is done in the receive callback
  delay(100);
}
