/**
 * @file report_task.c
 * @brief Tarea encargada del envío de datos de BPM mediante ESP-NOW.
 */

#include "report_task.h"
#include "config.h"
#include "espnow_module.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Tarea de reporte de datos.
 * 
 * Consume los valores de BPM desde la cola y los envía mediante ESP-NOW.
 * Solo transmite datos cuando se detecta la presencia del dedo en el sensor.
 * 
 * @param pv Parámetro no utilizado.
 */
void task_report(void *pv) {

    float data;

    while (1) {

        if (!fingerDetected) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        while (xQueueReceive(bpmQueue, &data, 0) == pdTRUE) {
            printf("BPM: %.2f\n", data);
            sendData(data);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}