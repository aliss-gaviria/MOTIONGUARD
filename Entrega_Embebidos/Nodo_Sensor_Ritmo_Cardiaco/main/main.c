/**
 * @file main.c
 * @brief Punto de entrada del sistema embebido para monitoreo de ritmo cardíaco.
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"
#include "driver/gpio.h"

#include "config.h"
#include "adc_module.h"
#include "espnow_module.h"
#include "sensor_task.h"
#include "report_task.h"
#include "state_machine.h"

/**
 * @brief Función principal del sistema.
 * 
 * Inicializa los recursos necesarios, configura periféricos y
 * crea las tareas del sistema bajo FreeRTOS.
 */
void app_main(void) {

    nvs_flash_init();

    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    init_adc();
    init_esp_now();

    bpmQueue = xQueueCreate(10, sizeof(float));

    xTaskCreate(task_sensor, "sensor", 4096, NULL, 5, NULL);
    xTaskCreate(task_report, "report", 4096, NULL, 5, NULL);
    xTaskCreate(state_machine_task, "fsm", 4096, NULL, 5, NULL);

    printf("Sistema iniciado\n");
}