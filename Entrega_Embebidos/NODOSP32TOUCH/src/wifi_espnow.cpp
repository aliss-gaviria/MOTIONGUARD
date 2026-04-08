#include "wifi_espnow.h"
#include "config.h"
#include "sd_card.h"
#include <ArduinoJson.h>

/**
 * @file espnow.cpp
 * @brief Implementación de la comunicación inalámbrica mediante ESP-NOW.
 *
 * @details
 * Este módulo se encarga de:
 * - Recibir datos desde otros nodos mediante el protocolo ESP-NOW.
 * - Procesar mensajes en formato JSON.
 * - Identificar el tipo de nodo emisor (Nodo 2 o Nodo 3).
 * - Extraer variables físicas (aceleración, giroscopio, temperatura, BPM, batería).
 * - Almacenar los datos recibidos en la tarjeta SD con marca de tiempo.
 * - Actualizar variables globales protegidas por mutex (ej: batería).
 *
 * Flujo de funcionamiento:
 * 1. ESP-NOW recibe un paquete de datos.
 * 2. Se copia a un buffer local seguro.
 * 3. Se busca el inicio del JSON.
 * 4. Se deserializa usando ArduinoJson.
 * 5. Se identifica el nodo según los campos presentes.
 * 6. Se formatea la información.
 * 7. Se guarda en la SD.
 */

void on_data_recv(const uint8_t *mac, const uint8_t *data, int len)
{
    char msg[256];

    if (len >= sizeof(msg)) len = sizeof(msg) - 1;

    memcpy(msg, data, len);
    msg[len] = '\0';

    Serial.println("\n===== MENSAJE RECIBIDO =====");
    Serial.println(msg);

    const char* jsonStart = strchr(msg, '{');

    if (jsonStart == NULL) {
        Serial.println("No se encontró JSON");
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStart);

    if (error)
    {
        Serial.print("Error JSON: ");
        Serial.println(error.c_str());
        return;
    }

    // ===== NODO 2 =====
    /**
     * Nodo 2 envía:
     * - Aceleración (x, y, z)
     * - Giroscopio (x, y, z)
     * - Temperatura
     */
    if (doc["accel_x"].is<float>())
    {
        float ax = doc["accel_x"];
        float ay = doc["accel_y"];
        float az = doc["accel_z"];
        float gx = doc["gyro_x"];
        float gy = doc["gyro_y"];
        float gz = doc["gyro_z"];
        float temp = doc["temp_c"];

        Serial.println("son del nodo 2");

        char buffer[200];

        snprintf(buffer, sizeof(buffer),
                 "NODO2 | ACC: %.4f %.4f %.4f | GYR: %.4f %.4f %.4f | TEMP: %.2f",
                 ax, ay, az, gx, gy, gz, temp);

        guardarDatoIndividual(buffer);
    }

       // ===== NODO 3 =====
       /**
       * Nodo 3 envía:
       * - BPM (frecuencia cardíaca)
       * - Voltaje
       * - Nivel de batería
       */
       else if (doc["BPM"].is<float>())
       {
        float bpm = doc["BPM"];
        float volt = doc["voltaje"];
        float bat = doc["bateria"];

        /**
         * Sección crítica protegida con mutex
         * para evitar condiciones de carrera con la tarea LCD
         */
        if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            bateria = bat;
            xSemaphoreGive(xDisplayMutex);
        }

        char buffer[150];

        snprintf(buffer, sizeof(buffer),
                 "NODO3 | BPM: %.2f | V: %.2f | BAT: %.2f",
                 bpm, volt, bat);

        guardarDatoIndividual(buffer);
    }
}