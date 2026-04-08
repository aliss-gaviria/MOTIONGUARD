/**
 * @file sensor.cpp
 * @brief Tarea FreeRTOS para adquisición de datos del sensor IMU.
 *
 * @details
 * Este módulo:
 * - Lee periódicamente acelerómetro y giroscopio
 * - Escala los datos físicos
 * - Imprime resultados por serial
 * - Guarda los datos en la SD
 * - Comparte información con otras tareas mediante mutex
 *
 * Frecuencia:
 * Determinada por INTERVALO_MUESTREO
 */

#include "sensor.h"
#include "config.h"
#include "sd_card.h"

/**
 * @brief Tarea encargada de la lectura periódica del sensor.
 *
 * @param pvParameters Parámetros de FreeRTOS (no utilizados).
 *
 * @note
 * Se ejecuta en un núcleo dedicado del ESP32.
 *
 * @warning
 * No bloquear la tarea demasiado tiempo para evitar watchdog.
 */
void TaskSensor(void *pvParameters)
{
    unsigned long ultimoTiempo = 0;

    while (true)
    {
        if (millis() - ultimoTiempo >= INTERVALO_MUESTREO)
        {
            ultimoTiempo = millis();

            QMI8658_read_xyz(acc, gyro, &tim_count);

            Serial.println("===== SENSOR LOCAL =====");
            Serial.printf("ACC: %.2f %.2f %.2f\n", acc[0], acc[1], acc[2]);
            Serial.printf("GYR: %.2f %.2f %.2f\n", gyro[0], gyro[1], gyro[2]);
            Serial.println("========================\n");

            char buffer[200];

            snprintf(buffer, sizeof(buffer),
                     "LOCAL | ACC: %.4f %.4f %.4f | GYR: %.4f %.4f %.4f",
                     acc[0]/ACC_SCALE,
                     acc[1]/ACC_SCALE,
                     acc[2]/ACC_SCALE,
                     gyro[0]/GYRO_SCALE,
                     gyro[1]/GYRO_SCALE,
                     gyro[2]/GYRO_SCALE);

            guardarDatoIndividual(buffer);

            if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
            {
                memcpy(acc, acc, sizeof(acc));
                memcpy(gyro, gyro, sizeof(gyro));
                xSemaphoreGive(xDisplayMutex);
            }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}