
#include <ESP8266WiFi.h>
#include <espnow.h>


// has to be changed for each device
#define ANCHOR_ID 3 // 1, 2, or 3


struct Packet{
    uint8_t id;
};

Packet pkt;

unsigned long packetCount = 0;

void setup(){

    Serial.begin(115200);
    Serial.println();
    Serial.println("ESP12E Anchor Starting...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    delay(500);

    // force channel 1 to match esp 32
    wifi_set_channel(1);
  
    Serial.print("WiFi Channel = ");
    Serial.println(wifi_get_channel());



    if (esp_now_init() != 0){
        Serial.println("ESP-NOW init FAILED");
        return;
    }

    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);

    // Broadcast address

    uint8_t broadcastAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; // To everything

    esp_now_add_peer( broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

    pkt.id = ANCHOR_ID;

    Serial.print("Anchor ID = ");
    Serial.println(pkt.id);

}

void loop() {

    esp_now_send(NULL, (uint8_t*)&pkt, sizeof(pkt));
    packetCount++;

    if(packetCount % 50 == 0) {
        Serial.print("Packets sent: ");
        Serial.println(packetCount);
    }
  

    delay(30); // 30 ms = ideal

}