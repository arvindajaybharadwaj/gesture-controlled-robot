#include "esp_now.h"
#include "WiFi.h"
#include "Wire.h"
#include "MPU6050_light.h"

MPU6050 mpu(Wire);

// Receiver MAC Address
uint8_t broadcastAddress[] = {0xD4, 0xE9, 0xF4, 0xB2, 0x3E, 0xF4};

typedef struct {
  bool f;
  bool b;
  bool l;
  bool r;
} message;

message data;

unsigned long timer = 0;
float x, y;

bool front = false, back = false, left = false, right = false;

esp_now_peer_info_t peerInfo;

void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != 0) {
    Serial.println("Peer add failed");
    return;
  }

  Wire.begin(21, 22);

  byte status = mpu.begin();
  if (status != 0) {
    Serial.println("MPU6050 failed!");
    while (1);
  }

  Serial.println("Calibrating...");
  delay(1000);
  mpu.calcOffsets();
  Serial.println("Done!");
}

void loop() {
  mpu.update();

  if (millis() - timer > 1000) {
    x = mpu.getAngleX();
    y = mpu.getAngleY();

    front = back = left = right = false;

    if (x >= 30) back = true;
    else if (x <= -30) front = true;
    else if (y >= 30) right = true;
    else if (y <= -30) left = true;

    data.f = front;
    data.b = back;
    data.l = left;
    data.r = right;

    Serial.print("X: "); Serial.print(x);
    Serial.print(" Y: "); Serial.print(y);
    Serial.print(" | F: "); Serial.print(front);
    Serial.print(" B: "); Serial.print(back);
    Serial.print(" L: "); Serial.print(left);
    Serial.print(" R: "); Serial.println(right);

    esp_now_send(broadcastAddress, (uint8_t *) &data, sizeof(data));

    timer = millis();
  }
}