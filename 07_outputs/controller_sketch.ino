/*
 * ESP32 (Computer) - Connected to Laptop
 * Receives data from python app and sends to ESP32 (Motor) with ESP-NOW
 */

#include <esp_now.h>
#include <WiFi.h>

uint8_t motorESP32Address[] = {0x94, 0xA9, 0x90, 0x78, 0x34, 0x38}; 

// Structure to send data
typedef struct struct_message {
  char command[20];
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// Callback when data is sent
void OnDataSent(const wifi_tx_info_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  
  // Set device as a Wifi station
  WiFi.mode(WIFI_STA);
  
  // Print MAC address for reference
  Serial.print("Computer ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register motor ESP32
  memcpy(peerInfo.peer_addr, motorESP32Address, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer (motor ESP32)     
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  
  Serial.println("Receiver ready. Waiting for commands...");
}

void loop() {
  // Check if data is available from serial
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Copy command to struct
    command.toCharArray(myData.command, sizeof(myData.command));
    
    // Send message by ESP-NOW to motor ESP32
    esp_err_t result = esp_now_send(motorESP32Address, (uint8_t *) &myData, sizeof(myData));
    
    if (result == ESP_OK) {
      Serial.println("Sent: " + command);
    } else {
      Serial.println("Error sending data");
    }
  }
}