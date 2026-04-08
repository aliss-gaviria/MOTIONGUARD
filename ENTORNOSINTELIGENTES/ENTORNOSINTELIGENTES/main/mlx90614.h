/**
 * @file mlx90614.h
 * @brief Declaraciones públicas del módulo de manejo del sensor MLX90614.
 *
 * Este archivo define la interfaz pública del módulo encargado de la
 * adquisición de temperatura mediante el sensor infrarrojo MLX90614.
 *
 * A través de estas funciones, otros módulos del sistema pueden:
 * - inicializar el bus I2C del sensor
 * - leer una temperatura desde un registro específico
 * - ejecutar una ventana completa de monitoreo térmico
 *
 * Su objetivo es separar la declaración de funciones de su implementación,
 * manteniendo organizado el código del proyecto.
 */

#ifndef MLX90614_H
#define MLX90614_H

#include <stdbool.h>
#include "esp_err.h"
#include "app_types.h"

/* ============================================================
 * FUNCIONES PÚBLICAS DEL MÓDULO MLX90614
 * ============================================================ */

/**
 * @brief Inicializa el bus I2C utilizado por el sensor MLX90614.
 *
 * @return ESP_OK si la inicialización del bus fue exitosa.
 * @return Código de error ESP-IDF si ocurre una falla.
 *
 * Esta función configura el puerto I2C, los pines asociados y el
 * controlador necesario para establecer la comunicación con el sensor.
 */
esp_err_t i2c_inicializar_mlx90614(void);

/**
 * @brief Lee una temperatura desde un registro del sensor MLX90614.
 *
 * @param registro Registro interno del sensor que se desea consultar.
 * @param temperatura_celsius Puntero donde se almacenará la temperatura leída.
 * @return ESP_OK si la lectura fue exitosa.
 * @return ESP_ERR_INVALID_ARG si el puntero de salida es inválido.
 * @return Código de error ESP-IDF si ocurre una falla en la lectura.
 *
 * Esta función permite obtener tanto la temperatura del objeto como
 * la temperatura ambiente, dependiendo del registro solicitado.
 */
esp_err_t mlx90614_leer_temperatura(uint8_t registro, float *temperatura_celsius);

/**
 * @brief Ejecuta una ventana completa de monitoreo de temperatura.
 *
 * @param modo_alerta Indica si la adquisición debe realizarse en modo
 * normal o en modo alerta, ajustando la cantidad de muestras.
 * @param resultado Puntero a la estructura donde se almacenará el resultado
 * procesado del monitoreo térmico.
 *
 * Esta función realiza múltiples lecturas del sensor MLX90614 y genera
 * un resultado promedio que puede ser usado por la lógica del sistema
 * para evaluar condiciones térmicas.
 */
void tarea_monitorear_temperatura(bool modo_alerta, ResultadoTemperatura_t *resultado);

#endif