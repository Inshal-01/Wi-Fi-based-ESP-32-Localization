#include <WiFi.h>
#include <esp_now.h>
#include <math.h>
#include "esp_wifi.h"


struct Anchor {
  uint8_t mac[6];
  float x;
  float y;
  float estimate = 0;
  float errorCov = 1;
  float filteredRSSI = 0;
  float distance = 0;
  bool updated = false;
};


Anchor anchors[3] = {
  {
    {
      0xF4,
      0xCF,
      0xA2,
      0x76,
      0xB1,
      0x01
    }, 0.0 f, 0.0 f
  },
  {
    {
      0xA8,
      0x48,
      0xFA,
      0xD4,
      0x27,
      0x9F
    },
    0.0 f,
    0.70 f
  },
  {
    {
      0x8C,
      0xAA,
      0xB5,
      0x1A,
      0x08,
      0x8A
    },
    0.70 f,
    0.0 f
  }
};



// Kalman Filter

float kalman_update(Anchor & a, float measurement) {
  const float Q = 0.05 f;
  const float R = 9.0 f; // process noise
  // measurement noise (RSSI noisy)
  float x_pred = a.estimate;
  float P_pred = a.errorCov + Q;
  float K = P_pred / (P_pred + R);
  a.estimate = x_pred + K * (measurement - x_pred);
  a.errorCov = (1 - K) * P_pred;
  return a.estimate;
}

// Neural Network (RSSI -> distance)

float sigmoid(float x) {
  x = constrain(x, -50, 50);
  return 1.0 f / (1.0 f + expf(-x));
}
float predict_distance(float rssi) {
  float W1[5] = {
    4.2465979,
    -8.21334,
    0.439348,
    0.92297,
    -12.41035
  };
  float b1[5] = {
    -5.974766,
    3.345039,
    -0.7674117,
    -3.0098816,
    2.2622246
  };
  float W2[5] = {
    12.7616568,
    4.6752659,
    -2.8485643,
    -13.6410193,
    -1.8993355
  };
  
  float b2 = 0.20820478;
  float x_norm = (rssi + 100.0 f) / 100.0 f;
  
  x_norm = constrain(x_norm, 0, 1);
  
  float a1[5];
  
  // the 5 inner activations
  for (int i = 0; i < 5; i++)
    a1[i] = sigmoid(x_norm * W1[i] + b1[i]);
  
  float out = b2;
  for (int i = 0; i < 5; i++)
    out += a1[i] * W2[i];

  if (out < 0) out = 0.01;
  if (out > 5.0) out = 5.0; 
  return out;
}


// Trilateration
bool trilaterate(float & x, float & y) {
  
    Anchor & A = anchors[0];
    Anchor & B = anchors[1];
    Anchor & C = anchors[2];

    float A1 = 2 * (B.x - A.x);
    float B1 = 2 * (B.y - A.y);
    float C1 = A.distance * A.distance - B.distance * B.distance -
    A.x * A.x + B.x * B.x -
    A.y * A.y + B.y * B.y;

    float A2 = 2 * (C.x - A.x);
    float B2 = 2 * (C.y - A.y);
    float C2 = A.distance * A.distance - C.distance * C.distance -
    A.x * A.x + C.x * C.x -
    A.y * A.y + C.y * C.y;

    float det = A1 * B2 - A2 * B1;

    if (fabs(det) < 0.0001 f)
        return false;

    x = (C1 * B2 - C2 * B1) / det;
    y = (A1 * C2 - A2 * C1) / det;

  return true;
}


bool allUpdated() {
  for (int i = 0; i < 3; i++)
    if (!anchors[i].updated)
      return false;
  return true;
}


void onReceive(const esp_now_recv_info * info, const uint8_t * data, int len) 
{
    int rssi = info -> rx_ctrl -> rssi;
  
    for (int i = 0; i < 3; i++) {
    
        if (memcmp(info -> src_addr, anchors[i].mac, 6) == 0) {
        
            float filtered = kalman_update(anchors[i], rssi);
            
            anchors[i].filteredRSSI = filtered;
            anchors[i].distance = predict_distance(filtered);
            anchors[i].updated = true;
        
            break;
        }
    }
  
  // Only trilaterate when all anchors updated
    if (allUpdated()) {
        float x, y;
        if (trilaterate(x, y)) {
            Serial.printf("Position: %.2f , %.2f\n", x, y);
        }
    // reset flags
    for (int i = 0; i < 3; i++)
        anchors[i].updated = false;
    }
}


void setup() {

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    //  Force same Channel AS ESP12E
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    Serial.println("ESP32 Receiver Started");
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }
    esp_now_register_recv_cb(onReceive);
}



void loop() {}