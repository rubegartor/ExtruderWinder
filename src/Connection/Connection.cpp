#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Connection/Connection.h>

typedef struct puller_message
{
    uint16_t revs;
    uint32_t speed;
} puller_message;

puller_message pullerMessage;

typedef struct winder_message
{
    uint16_t spoolSpeed;
} winder_message;

winder_message winderMessage;

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    memcpy(&pullerMessage, incomingData, sizeof(pullerMessage));
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Puller Speed: ");
    Serial.println(pullerMessage.speed);
    Serial.print("Puller revs: ");
    Serial.println(pullerMessage.revs);
    Serial.println();
}

uint8_t addr[] = {0xC8, 0xF0, 0x9E, 0xA2, 0x91, 0xF8};

void ESPNowConnection::init()
{
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Init error (ESP-Now)");

        while (1)
            ;
    }

    esp_now_register_recv_cb(OnDataRecv);

    esp_now_peer_info_t slaveInfo;
    memset(&slaveInfo, 0, sizeof(slaveInfo));
    memcpy(slaveInfo.peer_addr, addr, 6);
    slaveInfo.channel = 0;
    slaveInfo.encrypt = false;

    // Add slave
    if (esp_now_add_peer(&slaveInfo) != ESP_OK)
    {
        return;
    }
}

bool ESPNowConnection::send()
{
    esp_err_t result = esp_now_send(addr, (uint8_t *)&winderMessage, sizeof(winderMessage));

    return result == ESP_OK;
}

void setSpoolSpeed(uint16_t speed)
{
    winderMessage.spoolSpeed = speed;
}