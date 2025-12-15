/*
 * ESP32 (Motor) - Connected to Motors
 * Receives commands from ESP32 (Computer) by ESP-NOW to control motors
 */

#include <esp_now.h>
#include <WiFi.h>

// L298N Driver 1 - Controls Left Side Motors
const int FRONT_LEFT_PIN1 = D8;
const int FRONT_LEFT_PIN2 = D7;

const int BACK_LEFT_PIN1 = D10;
const int BACK_LEFT_PIN2 = D9;

// L298N Driver 2 - Controls Right Side Motors
const int FRONT_RIGHT_PIN1 = D6;
const int FRONT_RIGHT_PIN2 = D5;

const int BACK_RIGHT_PIN1 = D6;
const int BACK_RIGHT_PIN2 = D5;

// Motor speed settings (0-255)
const int FULL_SPEED = 200;       // Maximum speed for forward/backward
const int TURN_OUTER_SPEED = 200; // Outer wheels during turn - full speed
const int TURN_INNER_SPEED = 200; // Inner wheels during turn - used to be slower

// Structure to receive data (matches sender)
typedef struct struct_message {
  char command[20];
} struct_message;

struct_message incomingData;

// Callback function executed when ESP-NOW data is received
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  
  Serial.print("Command received: ");
  Serial.println(incomingData.command);
  
  String command = String(incomingData.command);
  
  // Execute the appropriate motor command
  if (command == "FORWARD") {
    moveForward();
  } else if (command == "BACKWARD") {
    moveBackward();
  } else if (command == "LEFT") {
    turnLeft();
  } else if (command == "RIGHT") {
    turnRight();
  } else if (command == "STOP") {
    stopMotors();
  }
}

void setup() {
  Serial.begin(115200);
  
  // Set motors as output
  pinMode(FRONT_LEFT_PIN1, OUTPUT);
  pinMode(FRONT_LEFT_PIN2, OUTPUT);
  pinMode(BACK_LEFT_PIN1, OUTPUT);
  pinMode(BACK_LEFT_PIN2, OUTPUT);
  pinMode(FRONT_RIGHT_PIN1, OUTPUT);
  pinMode(FRONT_RIGHT_PIN2, OUTPUT);
  //pinMode(BACK_RIGHT_PIN1, OUTPUT);
  //pinMode(BACK_RIGHT_PIN2, OUTPUT);

  // Configure LEDC PWM channels
  ledcAttach(FRONT_LEFT_PIN1, 5000, 8);
  ledcAttach(FRONT_LEFT_PIN2, 5000, 8);
  ledcAttach(BACK_LEFT_PIN1, 5000, 8);
  ledcAttach(BACK_LEFT_PIN2, 5000, 8);
  ledcAttach(FRONT_RIGHT_PIN1, 5000, 8);
  ledcAttach(FRONT_RIGHT_PIN2, 5000, 8);
  //ledcAttach(BACK_RIGHT_PIN1, 5000, 8);
  //ledcAttach(BACK_RIGHT_PIN2, 5000, 8);
  
  // Set device as a Wifi station
  WiFi.mode(WIFI_STA);
  
  // Print MAC address for reference
  Serial.print("Motor ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register receive callback function
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("Car controller ready and waiting for commands!");
  stopMotors(); // Start with all motors stopped
}

void loop() {
  // Nothing needed here. ESP-NOW handles all commands.
}

// ====== MOTOR CONTROL FUNCTIONS ======

// Sets a motors speed/direction.
void setMotor(int pin1, int pin2, int speed, bool forward) {
  if (forward) {
    ledcWrite(pin1, speed);  // PWM on forward pin
    ledcWrite(pin2, 0);      // Backward pin off
  } else {
    ledcWrite(pin1, 0);      // Forward pin off
    ledcWrite(pin2, speed);  // PWM on backward pin
  }
}

// Move all wheels forward at full speed
void moveForward() {
  Serial.println(">>> Moving Forward");
  setMotor(FRONT_LEFT_PIN1, FRONT_LEFT_PIN2, FULL_SPEED, true);
  setMotor(BACK_LEFT_PIN1, BACK_LEFT_PIN2, FULL_SPEED, true);
  setMotor(FRONT_RIGHT_PIN1, FRONT_RIGHT_PIN2, FULL_SPEED, true);
  setMotor(BACK_RIGHT_PIN1, BACK_RIGHT_PIN2, FULL_SPEED, true);
}

// Move all wheels backward at full speed
void moveBackward() {
  Serial.println(">>> Moving Backward");
  setMotor(FRONT_LEFT_PIN1, FRONT_LEFT_PIN2, FULL_SPEED, false);
  setMotor(BACK_LEFT_PIN1, BACK_LEFT_PIN2, FULL_SPEED, false);
  setMotor(FRONT_RIGHT_PIN1, FRONT_RIGHT_PIN2, FULL_SPEED, false);
  setMotor(BACK_RIGHT_PIN1, BACK_RIGHT_PIN2, FULL_SPEED, false);
}

// Turn left - all wheels move forward, left side slower than right side
void turnLeft() {
  Serial.println(">>> Turning Left");
  setMotor(FRONT_LEFT_PIN1, FRONT_LEFT_PIN2, TURN_INNER_SPEED, false);
  setMotor(BACK_LEFT_PIN1, BACK_LEFT_PIN2, TURN_INNER_SPEED, false);
  setMotor(FRONT_RIGHT_PIN1, FRONT_RIGHT_PIN2, TURN_OUTER_SPEED, true);
  setMotor(BACK_RIGHT_PIN1, BACK_RIGHT_PIN2, TURN_OUTER_SPEED, true);
}

// Turn right - all wheels move forward, right side slower than left side
void turnRight() {
  Serial.println(">>> Turning Right");
  setMotor(FRONT_LEFT_PIN1, FRONT_LEFT_PIN2, TURN_OUTER_SPEED, true);
  setMotor(BACK_LEFT_PIN1, BACK_LEFT_PIN2, TURN_OUTER_SPEED, true);
  setMotor(FRONT_RIGHT_PIN1, FRONT_RIGHT_PIN2, TURN_INNER_SPEED, false);
  setMotor(BACK_RIGHT_PIN1, BACK_RIGHT_PIN2, TURN_INNER_SPEED, false);
}

// Stop all motors immediately
void stopMotors() {
  Serial.println(">>> Stopping All Motors");
  // Set all pins to 0
  ledcWrite(FRONT_LEFT_PIN1, 0);
  ledcWrite(FRONT_LEFT_PIN2, 0);
  ledcWrite(BACK_LEFT_PIN1, 0);
  ledcWrite(BACK_LEFT_PIN2, 0);
  ledcWrite(FRONT_RIGHT_PIN1, 0);
  ledcWrite(FRONT_RIGHT_PIN2, 0);
  ledcWrite(BACK_RIGHT_PIN1, 0);
  ledcWrite(BACK_RIGHT_PIN2, 0);
}