/**
 * @file max30100.h
 * @brief Declaraciones públicas del módulo de manejo del sensor MAX30100.
 *
 * Este archivo define la interfaz pública del módulo encargado de la
 * adquisición y procesamiento del sensor MAX30100.
 *
 * A través de estas funciones, otros módulos del sistema pueden:
 * - inicializar el bus I2C del sensor
 * - inicializar y configurar el MAX30100
 * - leer muestras crudas del FIFO
 * - ejecutar una ventana completa de monitoreo de ritmo cardíaco
 *
 * Su propósito es separar la declaración de funciones de su implementación,
 * facilitando la organización y reutilización del código.
 */

#ifndef MAX30100_H
#define MAX30100_H

#include <stdbool.h>
#include "esp_err.h"
#include "app_types.h"

/* ============================================================
 * FUNCIONES PÚBLICAS DEL MÓDULO MAX30100
 * ============================================================ */

/**
 * @brief Inicializa el bus I2C utilizado por el sensor MAX30100.
 *
 * @return ESP_OK si la inicialización del bus fue exitosa.
 * @return Código de error ESP-IDF si ocurre una falla.
 *
 * Esta función configura el puerto I2C, los pines asociados y el
 * controlador necesario para establecer la comunicación con el sensor.
 */
esp_err_t i2c_inicializar_max30100(void);

/**
 * @brief Inicializa y configura internamente el sensor MAX30100.
 *
 * @return ESP_OK si el sensor fue configurado correctamente.
 * @return Código de error ESP-IDF si la inicialización falla.
 *
 * Esta función verifica que el sensor responda correctamente por I2C
 * y luego aplica la configuración necesaria para comenzar a trabajar
 * en modo de monitoreo cardíaco.
 */
esp_err_t max30100_inicializar(void);

/**
 * @brief Lee una muestra cruda del sensor MAX30100.
 *
 * @param muestra Puntero a la estructura donde se almacenará la muestra leída.
 * @return ESP_OK si la lectura fue exitosa.
 * @return Código de error ESP-IDF si ocurre una falla o si el puntero es inválido.
 *
 * La muestra obtenida contiene los valores crudos de los canales IR y RED
 * provenientes del FIFO del sensor.
 */
esp_err_t max30100_leer_muestra(MuestraMax30100_t *muestra);

/**
 * @brief Ejecuta una ventana completa de monitoreo de ritmo cardíaco.
 *
 * @param modo_alerta Indica si la medición debe realizarse en modo normal
 * o en modo alerta, ajustando la cantidad de muestras adquiridas.
 * @param resultado Puntero a la estructura donde se almacenará el resultado
 * procesado del monitoreo.
 *
 * Esta función realiza la adquisición de muestras del MAX30100, aplica
 * el procesamiento de señal correspondiente y genera un resultado que
 * incluye BPM, SpO2 estimada, calidad de señal y estado de validez.
 */
void tarea_monitorear_ritmo(bool modo_alerta, ResultadoRitmo_t *resultado);

#endif