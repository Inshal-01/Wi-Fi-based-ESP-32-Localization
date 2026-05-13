#include <WiFi.h>
#include <esp_now.h>
#include <math.h>
#include "esp_wifi.h"

struct Anchor {

    uint8_t mac[6];

    float x;
    float y;

    float estimate = -75;
    float errorCov = 1;

    float filteredRSSI = -75;

    bool active = false;
};

Anchor anchors[3] = {

    {{0xF4,0xCF,0xA2,0x76,0xB1,0x01}, 0.0f, 0.0f},

    {{0xA8,0x48,0xFA,0xD4,0x27,0x9F}, 0.0f, 0.70f},

    {{0x8C,0xAA,0xB5,0x1A,0x08,0x8A}, 0.70f, 0.0f}
};


float kalman_update(Anchor &a, float measurement)
{
    const float Q = 0.05f;
    const float R = 9.0f;

    float x_pred = a.estimate;
    float P_pred = a.errorCov + Q;

    float K = P_pred / (P_pred + R);

    a.estimate = x_pred + K * (measurement - x_pred);
    a.errorCov = (1 - K) * P_pred;

    return a.estimate;
}

// RSSI to weight mapping

float rssiToWeight(float rssi)
{
    rssi = constrain(rssi, -90, -40);

    // exponential weighting gives strong direction bias
    return exp((rssi + 90.0f)/10.0f);
}

// ESP-NOW RECEIVE

void onReceive(const esp_now_recv_info *info, const uint8_t *data, int len)
{
    int rssi = info->rx_ctrl->rssi;

    for(int i=0;i<3;i++)
    {
        if(memcmp(info->src_addr, anchors[i].mac,6)==0)
        {
            anchors[i].filteredRSSI =
                kalman_update(anchors[i], rssi);

            anchors[i].active = true;

            Serial.printf("Anchor %d RSSI %.1f\n",
                          i+1,
                          anchors[i].filteredRSSI);

            break;
        }
    }
}

void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    Serial.println("Weighted Vector Localization Started");

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW init failed");
        return;
    }

    esp_now_register_recv_cb(onReceive);
}

void loop()
{
    float vx = 0;
    float vy = 0;
    float wsum = 0;

    for(int i=0;i<3;i++)
    {
        if(!anchors[i].active)
            return; // wait until all anchors seen

        float w = rssiToWeight(anchors[i].filteredRSSI);

        vx += w * anchors[i].x;
        vy += w * anchors[i].y;
        wsum += w;
    }

    float x_est = vx / wsum;
    float y_est = vy / wsum;

    Serial.printf("Estimated Position: %.2f , %.2f\n",
                  x_est, y_est);

    delay(200);
}