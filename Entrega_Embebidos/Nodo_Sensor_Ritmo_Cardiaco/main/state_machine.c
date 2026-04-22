/**
 * @file state_machine.c
 * @brief Implementación de la máquina de estados del sistema.
 */

#include "state_machine.h"
#include "config.h"
#include "espnow_module.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_timer.h"
#include "esp_sleep.h"

/**
 * @brief Tarea que implementa la máquina de estados del sistema.
 * 
 * Controla el flujo de operación entre los estados:
 * INIT, STABILIZING, MONITORING y SLEEPING. Gestiona las transiciones
 * en función de la detección del dedo y del tiempo transcurrido.
 * 
 * @param pv Parámetro no utilizado.
 */
void state_machine_task(void *pv)
{
    while (1)
    {
        int64_t now = esp_timer_get_time() / 1000;

        switch (currentState)
        {
            case INIT:
                if (fingerDetected)
                {
                    currentState = STABILIZING;
                    stateStartTime = now;
                    printf("STABILIZING\n");
                }
                break;

            case STABILIZING:
                if (!fingerDetected)
                {
                    currentState = INIT;
                }
                else if (now - stateStartTime > STABILIZATION_TIME)
                {
                    currentState = MONITORING;
                    stateStartTime = now;

                    xQueueReset(bpmQueue);
                    sumaVoltaje = 0;
                    contadorMuestras = 0;

                    printf("MONITORING\n");
                }
                break;

            case MONITORING:
                if (!fingerDetected)
                {
                    currentState = INIT;
                }
                else if (now - stateStartTime > MONITORING_TIME)
                {
                    currentState = SLEEPING;
                    printf("SLEEPING\n");
                }
                break;

            case SLEEPING:
                sendData(bpmAvg);
                vTaskDelay(pdMS_TO_TICKS(200));

                esp_sleep_enable_timer_wakeup(30 * 1000000);
                esp_deep_sleep_start();
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}