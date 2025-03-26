#include "Arduino.h"
#include "TFT_eSPI.h"
#include "define.h"

int x, y, z;
int tipTemperature = 0;
int heaterTemperature = 0;
bool lightSwitch = false;

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
            if (tft == nullptr) {
                Serial.println("Error: tft is NULL");
                return;
            }
            digitalWrite(POWER_BUTTON, HIGH);
           
            tft->fillRectHGradient(0, 0, tft->width(), tft->height(), 0x05BF, 0x04D1);
            tft->setTextColor(TFT_WHITE);
            tft->setTextDatum(MC_DATUM);
            tft->setTextSize(2);
            tft->drawCentreString("Solder Station", SCREEN_WIDTH / 2, 40, 4);
        
            char buffer[30];
            sprintf(buffer, "Tip Temp: %d C", tipTemperature);
            tft->drawCentreString(buffer, SCREEN_WIDTH / 2, 100, 2);
        
            sprintf(buffer, "Heater Temp: %d C", heaterTemperature);
            tft->drawCentreString(buffer, SCREEN_WIDTH / 2, 140, 2);
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
};