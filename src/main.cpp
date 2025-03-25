#include <Arduino.h>
#include <Wire.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "SPI.h"
#include "TFT_eSPI.h"
#include "Adafruit_GFX.h"

#define LED1 2
#define HEATER_PWM 25
#define TIP_PWM 33
#define POWER_BUTTON 32
#define LIGHT_PIN 4
#define VOLTAGE_PIN 26
#define CURRENT_PIN 27
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define TFT_ROTATION -1

const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE HTML>
    <html lang="uk">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
        <title>LabSpace</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                text-align: center;
                margin: 0; padding: 0; height: 100vh;
                display: flex; flex-direction: column;
                background: linear-gradient(-45deg, #ee7752, #e73c7e, #23a6d5, #23d5ab);
                background-size: 400% 400%;
                animation: gradient 15s ease infinite;
            }
            @keyframes gradient {
                0% { background-position: 0% 50%; }
                50% { background-position: 100% 50%; }
                100% { background-position: 0% 50%; }
            }
            nav { display: flex; justify-content: space-around; background: linear-gradient(180deg, rgba(0,119,255,1) 20%, rgba(255,255,255,0) 100%); padding: 10px; }
            nav button { background: none; border: none; color: white; font-size: 16px; padding: 10px; cursor: pointer; }
            .page { display: none; flex-grow: 1; flex-direction: column; padding: 1%; min-height: calc(100vh - 50px); }
            .active { display: flex; }
            .grid-container { display: grid; grid-template-columns: 1fr 1fr; gap: 5px; width: 100%; flex-grow: 1; align-items: stretch; justify-content: center; }
            .grid-item {
                background: rgba(240, 248, 255, 0.411);
                color: white;
                padding: 20px;
                border-radius: 10px;
                font-size: 18px;
                text-align: center;
                cursor: pointer;
                display: flex;
                align-items: center;
                justify-content: center;
                transition: background 0.3s ease-in-out;
            }
            .grid-item:hover { background: rgba(0, 102, 255, 0.7); }
            .control-container { display: flex; flex-direction: column; align-items: center; margin-top: 20px; }
        </style>
    </head>
    <body>
        <nav>
            <button onclick="showPage('home')">Головна</button>
            <button onclick="showPage('settings')">Налаштування</button>
            <button onclick="showPage('about')">Автори</button>
        </nav>
        
        <div id="home" class="page active">
            <div class="grid-container">
                <div class="grid-item" onclick="showPage('0')">Паяльна станція</div>
                <div class="grid-item" onclick="showPage('1')">Осцилограф</div>
                <div class="grid-item" onclick="showPage('2')">Підсвітка</div>
                <div class="grid-item" onclick="showPage('3')">Блок живлення</div>
            </div>
        </div>
        
        <div id="0" class="page">
            <h1>Паяльна станція</h1>
            <div class="control-container">
                <label>Температура жала: <input type="range" min="0" max="255" id="tipTemp" oninput="sendPWM('tip', this.value)"></label>
                <label>Температура підігріву: <input type="range" min="0" max="255" id="heaterTemp" oninput="sendPWM('heater', this.value)"></label>
                <button onclick="togglePower()">Вкл/Викл</button>
            </div>
        </div>
    
        <div id="1" class="page">
            <h1>Осцилограф</h1>
            <p>Дані осцилографа будуть відображатися тут.</p>
        </div>
        
        <div id="2" class="page">
            <h1>Підсвітка</h1>
            <button onclick="toggleLight()">Увімкнути/Вимкнути</button>
        </div>
        
        <div id="3" class="page">
            <h1>Блок живлення</h1>
            <label>Напруга: <input type="range" min="0" max="30" id="voltage" oninput="sendPWM('voltage', this.value)"></label>
            <label>Сила струму: <input type="range" min="0" max="10" id="current" oninput="sendPWM('current', this.value)"></label>
        </div>
        
        <div id="settings" class="page">
            <h1>Налаштування</h1>
            <p>Тут можна змінити параметри.</p>
        </div>
        
        <div id="about" class="page">
            <h1>Автори</h1>
            <p>Інформація про розробників LabSpace.</p>
        </div>
        
        <script>
            let socket = new WebSocket("ws://" + location.host + "/ws");

            function showPage(pageId) {
                if (socket.readyState === WebSocket.OPEN) {
                    socket.send("menu:" + pageId);  
                } else {
                    console.error("WebSocket не підключений");
                }

                document.querySelectorAll('.page').forEach(page => {
                    page.classList.remove('active');
                    page.style.display = 'none';
                });

                let activePage = document.getElementById(pageId);
                if (activePage) {
                    activePage.classList.add('active');
                    activePage.style.display = 'flex';
                } else {
                    console.error("Сторінка з ID '" + pageId + "' не знайдена");
                }
            }

            socket.onmessage = function(event) {
                let msg = event.data;
                console.log("Отримано: " + msg);
            };

            function sendPWM(type, value) {
                socket.send(type + ":" + value);
            }

            function togglePower() {
                socket.send("toggle_power");
            }

            function toggleLight() {
                socket.send("toggle_light");
            }
        </script>
    </body>
    </html>
    
)rawliteral";

const char* ssid = "Sergey";
const char* password = "12031949";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
TFT_eSPI tft = TFT_eSPI();


TaskHandle_t TouchTaskHandle;

int x, y, z;
bool lightState = false;
int tipTemperature = 0;
int heaterTemperature = 0;

class DrawMenu {
    private:
        TFT_eSPI* tft;
        int btnWidth;
        int btnHeight;
        const char *labels[4] = {"Solder Station", "Oscillograph", "Light Control", "Power Supply"};
    
    public:
        DrawMenu(TFT_eSPI* display) {
            tft = display;
            tft->init();
            tft->setRotation(TFT_ROTATION); 
            btnWidth = SCREEN_WIDTH / 2;
            btnHeight = SCREEN_HEIGHT / 2;
        }
    
        void drawMainMenu() {
            tft->fillRectHGradient(0, 0, tft->width(), tft->height(), 0x05BF, 0x04D1);
            tft->setTextColor(TFT_GREEN);
            tft->setTextDatum(MC_DATUM);
            tft->setTextSize(1);
            
            for (int i = 0; i < 4; i++) {
                int x = (i % 2) * btnWidth;
                int y = (i / 2) * btnHeight;
                tft->drawRect(x, y, btnWidth, btnHeight, TFT_LIGHTGREY);
                tft->drawCentreString(labels[i], x + btnWidth / 2, y + btnHeight / 2, 2);
            }
        }

        void drawSolderStationMenu() {
            tft->fillScreen(TFT_BLACK);
            tft->setTextColor(TFT_WHITE);
            tft->setTextDatum(MC_DATUM);
            tft->setTextSize(2);
            tft->drawCentreString("Solder Station", SCREEN_WIDTH / 2, 40, 4);
            tft->drawCentreString("Tip Temp: " + String(tipTemperature) + " C", SCREEN_WIDTH / 2, 100, 2);
            tft->drawCentreString("Heater Temp: " + String(heaterTemperature) + " C", SCREEN_WIDTH / 2, 140, 2);
        }

        void drawMenuItem(int index) {
            if (index == 0) {
                drawSolderStationMenu();
            } else {
                tft->fillScreen(TFT_BLACK);
                tft->setTextColor(TFT_WHITE);
                tft->setTextDatum(MC_DATUM);
                tft->setTextSize(2);
                tft->drawCentreString(labels[index], SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 4);
            }
        }


};

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
            lightState = !lightState;
            digitalWrite(LIGHT_PIN, lightState);
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