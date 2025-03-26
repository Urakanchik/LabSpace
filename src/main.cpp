#include <Arduino.h>
#include <Wire.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "SPI.h"
#include "TFT_eSPI.h"
#include "Adafruit_GFX.h"
#include "esp_task_wdt.h"
#include "index.h"
#include "DrawMenu.h"
#include "define.h"

const char* ssid = "Sergey";
const char* password = "12031949";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

TFT_eSPI tft = TFT_eSPI();

TaskHandle_t IndicatorsUpdater;

DrawMenu menu(&tft);

void notifyClients(String message) {
    ws.textAll(message);
}

void onWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        String msg = "";
        for (size_t i = 0; i < len; i++) {
            msg += (char)data[i];
        }
        if (msg.startsWith("tip:")) {
            tipTemperature = msg.substring(4).toInt();
            analogWrite(TIP_PWM, tipTemperature);
        } else if (msg.startsWith("heater:")) {
            heaterTemperature = msg.substring(7).toInt();
            analogWrite(HEATER_PWM, heaterTemperature);
        } else if (msg == "toggle_power") {
            digitalWrite(POWER_BUTTON, !digitalRead(POWER_BUTTON));
        } else if (msg == "toggle_light") {
            lightSwitch = !lightSwitch;
            digitalWrite(LIGHT_PIN, lightSwitch);
        } else if (msg.startsWith("voltage:")) {
            int value = msg.substring(8).toInt();
            ledcWrite(2, value);
        } else if (msg.startsWith("current:")) {
            int value = msg.substring(8).toInt();
            ledcWrite(3, value);
        } else if (msg.startsWith("menu:")) {
            int index = msg.substring(5).toInt();
            menu.drawMenuItem(index);
        }
    }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_DATA) {
        onWebSocketMessage(arg, data, len);
    }
}


void initPins() {
    pinMode(LED1, OUTPUT);
    pinMode(POWER_BUTTON, OUTPUT);
    pinMode(LIGHT_PIN, OUTPUT);
    digitalWrite(LED1, HIGH);
    digitalWrite(POWER_BUTTON, LOW);
    digitalWrite(LIGHT_PIN, LOW);
    ledcSetup(0, 5000, 8);
    ledcSetup(1, 5000, 8);
    ledcSetup(2, 5000, 8);
    ledcSetup(3, 5000, 8);
    ledcAttachPin(HEATER_PWM, 0);
    ledcAttachPin(TIP_PWM, 1);
    ledcAttachPin(VOLTAGE_PIN, 2);
    ledcAttachPin(CURRENT_PIN, 3);
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println(WiFi.localIP());
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });
    initPins();
    server.begin(); 
    menu.drawMainMenu();
}

void loop() {
    ws.cleanupClients();
}