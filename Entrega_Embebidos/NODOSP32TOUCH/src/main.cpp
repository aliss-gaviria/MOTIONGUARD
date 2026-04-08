/**
 * @file main.cpp
 * @brief Punto de entrada del sistema embebido.
 *
 * @details
 * Este archivo:
 * - Inicializa todos los periféricos (WiFi, SD, LCD, sensores)
 * - Configura ESP-NOW
 * - Crea tareas FreeRTOS
 * - Gestiona comandos por Serial
 *
 * Arquitectura:
 * - Núcleo 0 → Sensor
 * - Núcleo 1 → LCD
 * - Callback → ESP-NOW
 */

#include "config.h"
#include "wifi_espnow.h"
#include "sensor.h"
#include "lcd.h"
#include <WiFi.h>
#include <esp_now.h>
#include "time.h"

// ===== DEFINICIÓN DE VARIABLES GLOBALES =====
SPIClass sdSPI(HSPI);

uint32_t Imagesize = LCD_1IN28_HEIGHT * LCD_1IN28_WIDTH * 2;
UWORD *BlackImage;

float acc[3], gyro[3];
float bateria = 0;
unsigned int tim_count = 0;

SemaphoreHandle_t xDisplayMutex;

CST816S touch(6, 7, 13, 5);

// ================= SETUP =================
void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin("RASPAHELADOS_2", "1061789083");

    unsigned long t0 = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
        delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
        configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    }

    esp_now_init();
    esp_now_register_recv_cb(on_data_recv);

    sdSPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    if (!SD.begin(SD_CS_PIN, sdSPI)) {
        Serial.println("Error SD");
    }

    xDisplayMutex = xSemaphoreCreateMutex();

    touch.begin();

    BlackImage = (UWORD *)malloc(Imagesize);

    DEV_Module_Init();
    LCD_1IN28_Init(HORIZONTAL);
    LCD_1IN28_Clear(WHITE);

    Paint_NewImage((UBYTE *)BlackImage, 240, 240, 0, WHITE);
    Paint_SetScale(65);
    Paint_Clear(WHITE);

    QMI8658_init();

    xTaskCreatePinnedToCore(TaskSensor, "Sensor", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TaskLCD, "LCD", 4096, NULL, 1, NULL, 1);
}

// ================= LOOP =================
void loop()
{
    if (Serial.available())
    {
        char cmd = Serial.read();

        if (cmd == 'd')
        {
            File file = SD.open("/datos.txt", FILE_READ);
            while (file.available()) Serial.write(file.read());
            file.close();
        }
        else if (cmd == 'b')
        {
            if (SD.exists("/datos.txt")) {
                SD.remove("/datos.txt");
                Serial.println("Archivo borrado");
            }
        }
    }
}