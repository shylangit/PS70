#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include <esp_now.h>
#include <WiFi.h>


// 00:4b:12:2f:c8:c0
uint8_t receiver1_address[] = {0x00, 0x4b, 0x12, 0x2f, 0xc8, 0xc0};
uint8_t receiver2_address[] = {0x00, 0x4b, 0x12, 0x2f, 0xc8, 0xc0}; // Change

typedef struct sensor_data {
  sensors_vec_t accel;
  sensors_vec_t gyro;
} sensor_data_t;

void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void add_peer(uint8_t* mac) {
  esp_now_peer_info_t peer_info = {};

  // Register peer
  memcpy(peer_info.peer_addr, mac, 6);
  peer_info.channel = 0;  
  peer_info.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peer_info) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("Successfully added peer");
}

sensor_data_t packet;
Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("ESP-NOW initialized");
  esp_now_register_send_cb(esp_now_send_cb_t(on_data_sent));

  add_peer(receiver1_address);
  add_peer(receiver2_address);

  // Initialize MPU
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 found");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
  
  delay(100);
}

void loop() {
  // Get new sensor events with the readings
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Update positions
  packet.accel = a.acceleration;
  packet.gyro = g.gyro;

  // Serial.println(packet.gyro.z);

  // Send message via ESP-NOW
  esp_err_t result;
  result = esp_now_send(receiver1_address, reinterpret_cast<uint8_t*>(&packet), sizeof(packet));
  result = esp_now_send(receiver2_address, reinterpret_cast<uint8_t*>(&packet), sizeof(packet));

  // Don't need to check result since OnDataSent handles output

  delay(100); // Not sure if we need to delay, experiment
}