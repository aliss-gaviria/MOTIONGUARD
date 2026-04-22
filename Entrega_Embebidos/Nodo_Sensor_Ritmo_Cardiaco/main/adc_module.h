/**
 * @file adc_module.h
 * @brief Interfaz del módulo ADC para lectura de voltaje y cálculo de batería.
 */

#ifndef ADC_MODULE_H
#define ADC_MODULE_H

#include "esp_adc/adc_oneshot.h"

/**
 * @brief Inicializa el ADC en modo one-shot.
 */
void init_adc(void);

/**
 * @brief Obtiene el voltaje real medido.
 * 
 * @return float Voltaje en voltios.
 */
float leer_voltaje(void);

/**
 * @brief Calcula el porcentaje de batería a partir del voltaje.
 * 
 * @param voltaje Voltaje medido.
 * @return float Porcentaje de batería (0 a 100).
 */
float calcular_porcentaje(float voltaje);

/**
 * @brief Handle del ADC compartido entre módulos.
 */
extern adc_oneshot_unit_handle_t adc_handle;

#endif