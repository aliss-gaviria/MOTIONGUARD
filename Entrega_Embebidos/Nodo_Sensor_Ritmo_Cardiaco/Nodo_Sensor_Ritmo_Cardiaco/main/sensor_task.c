/**
 * @file sensor_task.c
 * @brief Tarea encargada de la adquisición y procesamiento de la señal de pulso.
 */

#include "sensor_task.h"
#include "config.h"
#include "adc_module.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_timer.h"

/**
 * @brief Tarea de adquisición y procesamiento del sensor.
 * 
 * Realiza la lectura del ADC, aplica un filtro exponencial y detecta latidos
 * mediante umbral con histéresis. Calcula el BPM instantáneo y su promedio,
 * y envía los datos a una cola para su posterior transmisión.
 * 
 * Además, realiza el muestreo del voltaje de batería durante el estado
 * de monitoreo.
 * 
 * @param pv Parámetro no utilizado.
 */
void task_sensor(void *pv) {

    while (1) {

        int adc_raw;
        adc_oneshot_read(adc_handle, ADC_CANAL, &adc_raw);

        filtered = alpha * adc_raw + (1 - alpha) * filtered;

        int upper = threshold;
        int lower = threshold - hysteresis;

        bool current;

        if (filtered > upper) current = true;
        else if (filtered < lower) current = false;
        else current = lastState;

        if (current && !lastState && (esp_timer_get_time()/1000 - lastBeat > 300)) {

            unsigned long now = esp_timer_get_time()/1000;
            unsigned long delta = now - lastBeat;
            lastBeat = now;

            bpm = 60.0 / (delta / 1000.0);

            if (bpm > 40 && bpm < 200) {

                rates[rateSpot++] = bpm;
                rateSpot %= RATE_SIZE;

                bpmAvg = 0;
                int count = 0;

                for (int i = 0; i < RATE_SIZE; i++) {
                    if (rates[i] > 0) {
                        bpmAvg += rates[i];
                        count++;
                    }
                }

                if (count > 0) bpmAvg /= count;

                if (currentState == MONITORING) {
                    xQueueSend(bpmQueue, &bpmAvg, 0);
                }
            }
        }

        lastState = current;

        fingerDetected = (bpm > 0);

        gpio_set_level(LED_PIN, current);

        if (currentState == MONITORING) {

            sumaVoltaje += leer_voltaje();
            contadorMuestras++;

            if (contadorMuestras >= numMuestras) {
                voltajePromedio = sumaVoltaje / numMuestras;
                porcentajeBateria = calcular_porcentaje(voltajePromedio);

                sumaVoltaje = 0;
                contadorMuestras = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}