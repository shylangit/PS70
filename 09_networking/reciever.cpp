#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <esp_now.h>
#include <WiFi.h>

#include <ESP32Servo.h>

#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))
#define GYRO_MAX 15
#define GYRO_MIN -15
#define GYRO_DEADZONE 0.2
#define GYRO_PAN_SCALE 0.01
#define GYRO_TILT_SCALE -0.01
#define GYRO_TILT_OFFSET 0.5

float x_gyro_offset = 0;

class ServoWrapper {
  Servo servo;
  int servoPin;
  int startPos;

  public:
    ServoWrapper(int pin, int pos) {
      servoPin = pin;
      startPos = pos;
    }

    void attach() {
      servo.attach(servoPin);
      servo.write(startPos);
    }

    void move(int pos) {
      servo.write(pos);
    }
};

// Must match sender
typedef struct sensor_data {
  sensors_vec_t acceleration;
  sensors_vec_t gyro;
} sensor_data;

struct sensor_data packet;

ServoWrapper pan(32, 0);
ServoWrapper tilt(33, 0);

void OnDataRecv(const uint8_t * mac, const uint8_t *incoming_packet, int len) {
  memcpy(&packet, incoming_packet, sizeof(packet));
}

void setup() {
  Serial.begin(115200);
  
  pan.attach();
  tilt.attach();
  
  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

float pan_pos = 0.0;
float tilt_pos = 0.0;

void loop() {
  // Reduce drift with deadzones:
  if (abs(packet.gyro.x) < GYRO_DEADZONE) { packet.gyro.x = 0; }
  if (abs(packet.gyro.y) < GYRO_DEADZONE) { packet.gyro.y = 0; }  
  if (abs(packet.gyro.z) < GYRO_DEADZONE) { packet.gyro.z = 0; }  

  pan_pos += packet.gyro.z * GYRO_PAN_SCALE;
  tilt_pos += packet.gyro.x * GYRO_TILT_SCALE;

  // Clamp values
  pan_pos = CLAMP(pan_pos, GYRO_MIN, GYRO_MAX);
  tilt_pos = CLAMP(tilt_pos, GYRO_MIN, GYRO_MAX);

  Serial.printf("tilt_pos: %.2f\n", tilt_pos);

  pan.move(map(pan_pos, GYRO_MAX, GYRO_MIN, 0, 180));
  tilt.move(map(tilt_pos, GYRO_MAX, GYRO_MIN, 0, 180));
}
