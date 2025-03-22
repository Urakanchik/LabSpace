#include <Arduino.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "SPI.h"
#include "TFT_eSPI.h"
#include "XPT2046_Touchscreen.h"

#define LED1 2
#define HEATER_PWM 27
#define TIP_PWM 26
#define POWER_BUTTON 25
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

const char* ssid = "Sergey";
const char* password = "12031949";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
TFT_eSPI tft = TFT_eSPI();

class DrawMenu {
    private:
        TFT_eSPI* tft;
        int btnWidth;
        int btnHeight;
        const char *labels[4] = {"Solder Station", "Oscillograph", "Light", "Power Supply"};
    
    public:
        DrawMenu(TFT_eSPI* display) {
            tft = display;
            tft->init();
            tft->setRotation(1);
            btnWidth = SCREEN_WIDTH / 2;
            btnHeight = SCREEN_HEIGHT / 2;
        }
    
        void drawMainMenu() {
            tft->fillScreen(TFT_BLACK);
            tft->setTextColor(TFT_GREEN, TFT_BLACK);
            tft->setTextDatum(MC_DATUM);
            tft->setTextSize(1);
            
            for (int i = 0; i < 4; i++) {
                int x = (i % 2) * btnWidth;
                int y = (i / 2) * btnHeight;
                tft->drawRect(x, y, btnWidth, btnHeight, TFT_GREEN);
                tft->drawCentreString(labels[i], x + btnWidth / 2, y + btnHeight / 2, 2);
            }

        }
};

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
            int value = msg.substring(4).toInt();
            ledcWrite(0, value);
        } else if (msg.startsWith("heater:")) {
            int value = msg.substring(7).toInt();
            ledcWrite(1, value);
        } else if (msg == "toggle") {
            digitalWrite(POWER_BUTTON, !digitalRead(POWER_BUTTON));
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



DrawMenu menu(&tft);


// const ushort centerX = SCREEN_WIDTH / 2;
// const ushort centerY = SCREEN_HEIGHT / 2;






void initLedPins()
{
    pinMode(LED1, OUTPUT);
    pinMode(POWER_BUTTON, OUTPUT);
    digitalWrite(LED1, HIGH);
    digitalWrite(POWER_BUTTON, LOW);
    ledcSetup(0, 5000, 8);
    ledcSetup(1, 5000, 8);
    ledcAttachPin(HEATER_PWM, 0);
    ledcAttachPin(TIP_PWM, 1);
}

const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE HTML><html>
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
                <div class="grid-item" onclick="showPage('soldering')">Паяльна станція</div>
                <div class="grid-item">Осцилограф</div>
                <div class="grid-item">Підсвітка</div>
                <div class="grid-item">Блок живлення</div>
            </div>
        </div>
        
        <div id="soldering" class="page">
            <h1>Паяльна станція</h1>
            <div class="control-container">
                <label>Температура жала: <input type="range" min="0" max="255" id="tipTemp" oninput="sendPWM('tip', this.value)"></label>
                <label>Температура підігріву: <input type="range" min="0" max="255" id="heaterTemp" oninput="sendPWM('heater', this.value)"></label>
                <button onclick="togglePower()">Вкл/Викл</button>
            </div>
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
            function showPage(pageId) {
                document.querySelectorAll('.page').forEach(page => {
                    page.classList.remove('active');
                    page.style.display = 'none';
                });
                let activePage = document.getElementById(pageId);
                activePage.classList.add('active');
                activePage.style.display = 'flex';
            }
    
            let socket = new WebSocket("ws://" + location.host + "/ws");
    
            socket.onmessage = function(event) {
                let msg = event.data;
                if (msg.startsWith("tip:")) {
                    document.getElementById("tipTemp").value = msg.substring(4);
                } else if (msg.startsWith("heater:")) {
                    document.getElementById("heaterTemp").value = msg.substring(7);
                } else if (msg.startsWith("power:")) {
                    console.log("Паяльна станція " + (msg.substring(6) == "1" ? "увімкнена" : "вимкнена"));
                }
            };
    
            function sendPWM(type, value) {
                socket.send(type + ":" + value);
            }
    
            function togglePower() {
                socket.send("toggle");
            }
    
        </script>
    </body>
    </html>
    )rawliteral";

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

    initLedPins();

    server.begin(); 

    menu.drawMainMenu();

}

void loop() {
    ws.cleanupClients();
}


