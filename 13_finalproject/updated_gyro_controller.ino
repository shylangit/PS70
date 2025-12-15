/*
 * ESP32 (Controller) - With MPU6050 Gyroscope
 * Reads tilt data from MPU6050 and sends commands to ESP32 (Motor) with ESP-NOW
 * MPU6050 mounted on back of right hand with component side facing DOWN
 * Default position: fingers pointing DOWN (perpendicular to floor)
 */
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

uint8_t motorESP32Address[] = {0x58, 0x8C, 0x81, 0xA5, 0x46, 0x94};

// MPU6050 sensor object
Adafruit_MPU6050 mpu;

// Tilt thresholds (m/s^2)
const float TILT_THRESHOLD = 2.5;  // Minimum tilt to trigger movement
const float TILT_DEADZONE = 1.5;   // Deadzone for stopping (level detection)

// Structure to send data
typedef struct struct_message {
  char command[20];
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// Track last command to avoid sending duplicates
String lastCommand = "";

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  
  // Initialize MPU6050
  Serial.println("Initializing MPU6050...");
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  
  // Configure MPU6050
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);  // Smooth out readings
  
  Serial.println("MPU6050 configured successfully!");
  Serial.println("Default position: fingers pointing DOWN, component side facing DOWN");
  
  // Set device as a Wifi station
  WiFi.mode(WIFI_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register send callback
  esp_now_register_send_cb(reinterpret_cast<esp_now_send_cb_t>(OnDataSent));
  
  // Register motor ESP32
  memcpy(peerInfo.peer_addr, motorESP32Address, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer (motor ESP32)    
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("Controller ready. Tilt to control the car!");
  delay(100);
}

void loop() {
  // Get sensor readings
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  // Read acceleration values
  // y-axis: Reads gravity in default position (~-9.8)
  // x-axis: Forward/Backward control (tilt forward = more negative X)
  // z-axis: Left/Right control
  float accelX = a.acceleration.x;  // Forward/Backward tilt
  float accelY = a.acceleration.y;  // Gravity reference
  float accelZ = a.acceleration.z;  // Left/Right tilt
  
  // Determine command based on tilt
  String command = determineCommand(accelX, accelZ);
  
  // Only send if command has changed (to reduce traffic)
  if (command != lastCommand) {
    sendCommand(command);
    lastCommand = command;
  }
  
  // Print debug info
  Serial.print("Accel X: ");
  Serial.print(accelX, 2);
  Serial.print(" | Y: ");
  Serial.print(accelY, 2);
  Serial.print(" | Z: ");
  Serial.print(accelZ, 2);
  Serial.print(" | Command: ");
  Serial.println(command);
  
  delay(100);  // Read sensor 10 times per second
}

// Determine command based on accelerometer readings
// x-axis: Forward/Backward (fingers direction)
// z-axis: Left/Right (palm orientation)
String determineCommand(float accelX, float accelZ) {
  // Check if tilted enough in x axis (forward/backward)
  // Tilting forward makes x more negative
  // Tilting backward makes x more positive
  if (accelX < -TILT_THRESHOLD) {
    return "FORWARD";  // Tilt forward (fingers forward, negative x)
  }
  else if (accelX > TILT_THRESHOLD) {
    return "BACKWARD";  // Tilt backward (fingers back, positive x)
  }
  
  // Check if tilted enough in z-axis (left/right)
  // Tilt right makes z more postive
  // Tilt left makes z more negative
  if (accelZ > TILT_THRESHOLD) {
    return "RIGHT";  // Tilt right (palm down, positive z)
  }
  else if (accelZ < -TILT_THRESHOLD) {
    return "LEFT";  // Tilt left (palm up, negative z)
  }
  
  // If no significant tilt detected, stop
  return "STOP";
}

// Send command via ESP-NOW
void sendCommand(String command) {
  // Copy command to struct
  command.toCharArray(myData.command, sizeof(myData.command));
  
  // Send message by ESP-NOW to motor ESP32
  esp_err_t result = esp_now_send(motorESP32Address, (uint8_t *) &myData, sizeof(myData));
  
  if (result == ESP_OK) {
    Serial.println(">>> Sent: " + command);
  } else {
    Serial.println(">>> Error sending data");
  }
}