/**
 * @file adc_module.c
 * @brief Módulo para la adquisición de datos del ADC y cálculo de batería.
 */

#include "adc_module.h"
#include "config.h"

#include "esp_adc/adc_oneshot.h"

/**
 * @brief Handle del ADC en modo one-shot.
 */
adc_oneshot_unit_handle_t adc_handle;

/**
 * @brief Inicializa el ADC en modo one-shot.
 * 
 * Configura la unidad ADC1 y el canal definido para la lectura analógica.
 */
void init_adc(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc_handle, ADC_CANAL, &config);
}

/**
 * @brief Lee el voltaje real de la batería.
 * 
 * Realiza una lectura del ADC, la convierte a voltaje y aplica
 * el factor del divisor resistivo y calibración.
 * 
 * @return float Voltaje real de la batería en voltios.
 */
float leer_voltaje(void) {
    int adc_raw;
    adc_oneshot_read(adc_handle, ADC_CANAL, &adc_raw);

    float Vadc = (adc_raw / 4095.0) * 3.3;
    float Vreal = Vadc * ((R1 + R2) / R2) * CALIBRACION_V;

    return Vreal;
}

/**
 * @brief Calcula el porcentaje de batería a partir del voltaje.
 * 
 * @param voltaje Voltaje medido de la batería.
 * @return float Porcentaje de carga en el rango [0, 100].
 */
float calcular_porcentaje(float voltaje) {
    float p = (voltaje - VMIN) / (VMAX - VMIN) * 100.0;

    if (p > 100) p = 100;
    if (p < 0) p = 0;

    return p;
}