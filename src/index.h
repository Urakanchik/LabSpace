#include "Arduino.h"

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
            <button onclick="showPage('4')">Головна</button>
            <button onclick="showPage('5')">Налаштування</button>
            <button onclick="showPage('6')">Автори</button>
        </nav>
        
        <div id="4" class="page active">
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