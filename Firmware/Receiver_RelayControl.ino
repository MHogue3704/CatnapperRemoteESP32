#include <esp_now.h>
#include <WiFi.h>

#define RELAY_UP    2
#define RELAY_DOWN  3

typedef struct {
  bool up;
  bool down;
} ChairCommand;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  pinMode(RELAY_UP, OUTPUT);
  pinMode(RELAY_DOWN, OUTPUT);
  digitalWrite(RELAY_UP, LOW);
  digitalWrite(RELAY_DOWN, LOW);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);
}

void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  ChairCommand *cmd = (ChairCommand *)incomingData;
  digitalWrite(RELAY_UP, cmd->up ? HIGH : LOW);
  digitalWrite(RELAY_DOWN, cmd->down ? HIGH : LOW);
}

void loop() {
  // Nothing here
}
