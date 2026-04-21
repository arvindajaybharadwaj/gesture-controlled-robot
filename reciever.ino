#include <esp_now.h>
#include <WiFi.h>

// ==========================
// MOTOR PINS
// ==========================
#define IN1 18
#define IN2 19
#define IN3 23
#define IN4 32
#define ENA 5
#define ENB 33

// ==========================
// RECEIVED DATA STRUCT
// Must match transmitter
// ==========================
typedef struct {
  bool f;
  bool b;
  bool l;
  bool r;
} message;

message data;

// Track last movement to avoid unnecessary switching
String currentState = "STOP";

// ==========================
// SAFE MOTOR FUNCTIONS
// ==========================

// Stop both motors
void stopMotors() {
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// Safe direction change helper
void safeMove(
  bool in1, bool in2,
  bool in3, bool in4,
  int speedVal
) {
  // Stop PWM first
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
  delay(500);

  // Set direction
  digitalWrite(IN1, in1);
  digitalWrite(IN2, in2);
  digitalWrite(IN3, in3);
  digitalWrite(IN4, in4);
  delay(300);

  // Apply PWM
  ledcWrite(ENA, speedVal);
  ledcWrite(ENB, speedVal);
}

// Forward
void forward(int speedVal) {
  safeMove(HIGH, LOW, HIGH, LOW, speedVal);
}

// Backward
void backward(int speedVal) {
  safeMove(LOW, HIGH, LOW, HIGH, speedVal);
}

// Left turn: right motor only
void left(int speedVal) {
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
  delay(500);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(300);

  ledcWrite(ENA, 0);
  ledcWrite(ENB, speedVal);
}

// Right turn: left motor only
void right(int speedVal) {
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);
  delay(500);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  delay(300);

  ledcWrite(ENA, speedVal);
  ledcWrite(ENB, 0);
}

// ==========================
// ESP-NOW CALLBACK
// ==========================
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));

  Serial.print("Received -> ");
  Serial.print("F: "); Serial.print(data.f);
  Serial.print(" B: "); Serial.print(data.b);
  Serial.print(" L: "); Serial.print(data.l);
  Serial.print(" R: "); Serial.println(data.r);
}

// ==========================
// SETUP
// ==========================
void setup() {
  Serial.begin(115200);

  // Motor pin setup
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  ledcAttach(ENA, 1000, 8);
  ledcAttach(ENB, 1000, 8);

  stopMotors();

  // ESP-NOW setup
  WiFi.mode(WIFI_STA);
  delay(500);
  Serial.print("Mac Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Receiver Ready");
}

// ==========================
// LOOP
// ==========================
void loop() {

  // Forward
  if (data.f && currentState != "FORWARD") {
    Serial.println("Moving Forward");
    forward(120);
    currentState = "FORWARD";
  }

  // Backward
  else if (data.b && currentState != "BACKWARD") {
    Serial.println("Moving Backward");
    backward(120);
    currentState = "BACKWARD";
  }

  // Left
  else if (data.l && currentState != "LEFT") {
    Serial.println("Turning Left");
    left(120);
    currentState = "LEFT";
  }

  // Right
  else if (data.r && currentState != "RIGHT") {
    Serial.println("Turning Right");
    right(120);
    currentState = "RIGHT";
  }

  // Stop if no gesture
  else if (!data.f && !data.b && !data.l && !data.r && currentState != "STOP") {
    Serial.println("Stopping");
    stopMotors();
    currentState = "STOP";
  }

  delay(50);
}