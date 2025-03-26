#include "Arduino.h"
#include "TFT_eSPI.h"
#include "define.h"

int x, y, z;
int tipTemperature = 0;
int heaterTemperature = 0;
bool lightSwitch = false;
bool oscillographOpened = false;
bool solderOpened = false;

float amplitude = 50.0; 
float frequency = 0.03;  
float phase = 0.0;       
float yOffset = TFT_WIDTH/ 2; 

int prevX = 0;
int prevY = 0;

// Время последнего обновления
unsigned long lastUpdateTimeOsc = 0;
unsigned long lastUpdateTimeSol = 0;
const unsigned long updateIntervalOsc = 30; 
const unsigned long updateIntervalSol = 50; 



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

        
        void drawMenuItem(int index) {
            if (index < 0 || index >= 5) {
                Serial.println("Error: Invalid menu index");
                return;
            }
        
            switch (index) {
                case 0:
                    drawSolderStationMenu();
                    break;
                case 1:
                    drawOscilloscopeGrid();
                    break;
                case 4:
                    drawMainMenu();
                    break;
                default:
                    if (tft == nullptr) {
                        Serial.println("Error: tft is NULL");
                        return;
                    }
                    tft->fillRectHGradient(0, 0, tft->width(), tft->height(), 0x05BF, 0x04D1);
                    tft->setTextColor(TFT_WHITE);
                    tft->setTextDatum(MC_DATUM);
                    tft->setTextSize(2);
                    tft->drawCentreString(labels[index], SCREEN_WIDTH / 2, 100, 4);
                    break;
            }
        }
    
        void drawMainMenu() {
            oscillographOpened = false;
            solderOpened = false;
            digitalWrite(POWER_BUTTON, LOW);
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
            solderOpened = true;
            if (tft == nullptr) {
                Serial.println("Error: tft is NULL");
                return;
            }
            digitalWrite(POWER_BUTTON, HIGH);
            
            // Очистка экрана с цветом фона
            tft->fillScreen(0x05BF);
            
            // Рисуем статичные элементы (заголовок)
            tft->setTextColor(TFT_WHITE);
            tft->setTextDatum(MC_DATUM);
            tft->setTextSize(2);
            tft->drawCentreString("Solder Station", SCREEN_WIDTH / 2, 40, 4);
            
            // Рисуем подписи для температур (статичная часть)
            tft->setTextSize(2);
            tft->setTextDatum(TL_DATUM); // Выравнивание по левому краю
            
            // Подписи с фиксированными позициями
            tft->setCursor(20, 90);
            tft->print("Tip Temp:");
            
            tft->setCursor(20, 130);
            tft->print("Heater Temp:");
        }

        void updateSolder()
        {
            static char tipBuffer[10] = "";
            static char heaterBuffer[10] = "";
            static int lastTipTemp = -1;
            static int lastHeaterTemp = -1;
            
            // Обновляем только если температура изменилась
            if (tipTemperature != lastTipTemp) {
                // Стираем старую температуру
                tft->setTextColor(0x05BF); // Цвет фона
                tft->setTextDatum(TR_DATUM); // Выравнивание по правому краю
                tft->drawString(tipBuffer, SCREEN_WIDTH - 20, 90, 2);
                
                // Рисуем новую температуру
                snprintf(tipBuffer, sizeof(tipBuffer), "%d C", tipTemperature);
                tft->setTextColor(TFT_WHITE);
                tft->drawString(tipBuffer, SCREEN_WIDTH - 20, 90, 2);
                
                lastTipTemp = tipTemperature;
            }
            
            if (heaterTemperature != lastHeaterTemp) {
                // Стираем старую температуру
                tft->setTextColor(0x05BF); // Цвет фона
                tft->setTextDatum(TR_DATUM); // Выравнивание по правому краю
                tft->drawString(heaterBuffer, SCREEN_WIDTH - 20, 130, 2);
                
                // Рисуем новую температуру
                snprintf(heaterBuffer, sizeof(heaterBuffer), "%d C", heaterTemperature);
                tft->setTextColor(TFT_WHITE);
                tft->drawString(heaterBuffer, SCREEN_WIDTH - 20, 130, 2);
                
                lastHeaterTemp = heaterTemperature;
            }

        }
        
        void drawOscilloscopeGrid() {
            oscillographOpened = true;
            tft->fillRectHGradient(0, 0, tft->width(), tft->height(), 0x05BF, 0x04D1);
            for (int x = 0; x <= TFT_HEIGHT; x += GRID_X_STEP) {
                tft->drawFastVLine(x, 0, TFT_WIDTH, GRID_COLOR);
            }
            
            for (int y = 0; y <= TFT_WIDTH; y += GRID_Y_STEP) {
                tft->drawFastHLine(0, y, TFT_HEIGHT, GRID_COLOR);
            }
            
            // for (int x = 0; x <= TFT_HEIGHT; x += GRID_X_STEP/GRID_X_DIV) {
            //     tft->drawFastVLine(x, 0, TFT_WIDTH, (x % GRID_X_STEP == 0) ? GRID_COLOR : TFT_DARKGREY);
            // }
            
            // for (int y = 0; y <= TFT_WIDTH; y += GRID_Y_STEP/GRID_Y_DIV) {
            //     tft->drawFastHLine(0, y, TFT_HEIGHT, (y % GRID_Y_STEP == 0) ? GRID_COLOR : TFT_DARKGREY);
            // }
            
            tft->drawFastHLine(0, TFT_WIDTH/2, TFT_HEIGHT, AXIS_COLOR);
            tft->drawFastVLine(TFT_HEIGHT/2, 0, TFT_WIDTH, AXIS_COLOR);

            for (int x = 0; x <= TFT_HEIGHT; x += GRID_X_STEP) {
                char buf[6];
                sprintf(buf, "%d", (x - TFT_HEIGHT/2)*5); 
                tft->setTextColor(SCALE_COLOR);
                tft->setTextSize(1);
                tft->setCursor(x - 10, TFT_WIDTH - 15);
                tft->print(buf);
            }

            for (int y = 0; y <= TFT_WIDTH; y += GRID_Y_STEP) {
                char buf[6];
                sprintf(buf, "%.1f", (TFT_WIDTH/2 - y)/20.0); 
                tft->setTextColor(SCALE_COLOR);
                tft->setTextSize(1);
                tft->setCursor(5, y - 5);
                tft->print(buf);
            }

            tft->setTextColor(SCALE_COLOR);
            tft->setTextSize(1);
            tft->setCursor(TFT_HEIGHT - 30, TFT_WIDTH - 15);
            tft->print("ms");
            tft->setCursor(5, 5);
            tft->print("V");
        }

        void updateWave() {
            static int xPos = 0;
        
            float y = yOffset - amplitude * sin(2 * PI * frequency * xPos + phase);
            int yPos = constrain((int)y, 0, TFT_WIDTH-1);
            
            tft->drawPixel(xPos, yPos, WAVE_COLOR);
            
            prevX = xPos;
            prevY = yPos;
            
            xPos++;
            if (xPos >= TFT_HEIGHT) {
                xPos = 0;
                amplitude = 40 + random(40);
                frequency = 0.01 + random(10) / 500.0;
                phase += 0.1;
                drawOscilloscopeGrid();
            }
        }
};
