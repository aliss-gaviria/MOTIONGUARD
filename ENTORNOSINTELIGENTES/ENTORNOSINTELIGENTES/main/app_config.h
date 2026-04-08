/**
 * @file app_config.h
 * @brief Configuración global del sistema de monitoreo biomédico.
 *
 * Este archivo concentra todos los parámetros fijos del proyecto:
 * pines de conexión, puertos I2C, direcciones de sensores, tiempos
 * de muestreo, umbrales fisiológicos y tamaños de buffers.
 *
 * Su función principal es servir como punto central de configuración
 * para que los demás módulos del sistema trabajen con valores
 * consistentes y fáciles de modificar.
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include "driver/i2c.h"

/* ============================================================
 * CONFIGURACIÓN DE PINES
 * ============================================================ */
/**
 * @brief Asignación de pines físicos del sistema.
 *
 * En este bloque se definen los GPIO usados por los sensores y
 * elementos de interacción:
 * - MAX30100 para ritmo cardíaco
 * - MLX90614 para temperatura
 * - LED indicador
 * - botón de control
 *
 * Esta sección permite cambiar fácilmente el cableado lógico del
 * proyecto sin modificar la implementación interna de los módulos.
 */

// MAX30100 (ritmo cardíaco)
#define MAX30100_I2C_SDA_GPIO           15
#define MAX30100_I2C_SCL_GPIO           16

// MLX90614 (temperatura)
#define MLX90614_I2C_SDA_GPIO           18
#define MLX90614_I2C_SCL_GPIO           17

// LED y botón
#define LED_GPIO                        21
#define BUTTON_GPIO                     33

/* ============================================================
 * CONFIGURACIÓN I2C GENERAL
 * ============================================================ */
/**
 * @brief Parámetros generales de comunicación I2C.
 *
 * Aquí se definen la frecuencia base del bus, el tiempo máximo
 * de espera para transacciones y las direcciones I2C de los sensores.
 *
 * Estos valores son utilizados por las rutinas de inicialización
 * y lectura para garantizar una comunicación estable con el MAX30100
 * y el MLX90614.
 */

// Frecuencia estándar de I2C
#define I2C_MASTER_FREQ_HZ              100000

// Timeout general para transacciones I2C
#define I2C_TIMEOUT_MS                  1000

// Direcciones I2C de los sensores
#define MAX30100_I2C_ADDRESS            0x57
#define MLX90614_I2C_ADDRESS            0x5A

/* ============================================================
 * ALIAS PARA LOS MÓDULOS .C
 * ============================================================ */
/**
 * @brief Alias de configuración usados por los módulos de sensores.
 *
 * Este bloque crea nombres intermedios para que cada archivo `.c`
 * trabaje con identificadores más descriptivos y desacoplados de
 * los nombres base del hardware.
 *
 * Su lógica es facilitar la reutilización del código y hacer más
 * clara la configuración específica de cada módulo.
 */

// MAX30100
#define I2C_PUERTO_MAX30100             I2C_NUM_0
#define I2C_GPIO_SDA_MAX30100           MAX30100_I2C_SDA_GPIO
#define I2C_GPIO_SCL_MAX30100           MAX30100_I2C_SCL_GPIO
#define I2C_FRECUENCIA_MAX30100_HZ      I2C_MASTER_FREQ_HZ
#define MAX30100_DIRECCION_I2C          MAX30100_I2C_ADDRESS

// MLX90614
#define I2C_PUERTO_MLX90614             I2C_NUM_1
#define I2C_GPIO_SDA_MLX90614           MLX90614_I2C_SDA_GPIO
#define I2C_GPIO_SCL_MLX90614           MLX90614_I2C_SCL_GPIO
#define I2C_FRECUENCIA_MLX90614_HZ      I2C_MASTER_FREQ_HZ
#define MLX90614_DIRECCION_I2C          MLX90614_I2C_ADDRESS

/* ============================================================
 * CONFIGURACIÓN DE TIEMPOS
 * ============================================================ */
/**
 * @brief Parámetros temporales generales del sistema.
 *
 * Esta sección define duraciones de monitoreo y periodos de muestreo
 * genéricos para operación normal y operación en alerta.
 *
 * La lógica de estos parámetros es controlar la frecuencia con la
 * que el sistema adquiere datos y cuánto dura cada ventana de análisis.
 */

// Tiempo total de monitoreo por estado
#define TIEMPO_MONITOREO_MS             6000

// Períodos genéricos
#define MUESTREO_NORMAL_MS              200
#define MUESTREO_ALERTA_MS              100

/* ============================================================
 * CONFIGURACIÓN DE RITMO CARDÍACO
 * ============================================================ */
/**
 * @brief Umbrales fisiológicos para evaluación del ritmo cardíaco.
 *
 * Aquí se establece el rango considerado normal para la práctica.
 * Estos límites son usados por la lógica del sistema para determinar
 * si una medición de BPM debe generar una alerta roja.
 *
 * También se definen alias para mantener compatibilidad con otros
 * módulos del proyecto.
 */

// Rango normal fisiológico para tu práctica
#define RITMO_MIN_LPM                   55.0f
#define RITMO_MAX_LPM                   75.0f

// Alias usados por system_logic.c
#define UMBRAL_BPM_MINIMO               RITMO_MIN_LPM
#define UMBRAL_BPM_MAXIMO               RITMO_MAX_LPM

/* ============================================================
 * CONFIGURACIÓN DE TEMPERATURA
 * ============================================================ */
/**
 * @brief Umbrales y criterios de evaluación térmica.
 *
 * Este bloque define el valor de temperatura a partir del cual
 * puede activarse una alerta amarilla y el cambio mínimo entre
 * mediciones para considerar una variación importante.
 *
 * Su propósito es apoyar la lógica de detección de eventos térmicos
 * anormales dentro del sistema.
 */

// Umbral de temperatura para alerta amarilla
#define TEMPERATURA_UMBRAL_ALERTA_C     36.0f

// Cambio abrupto mínimo para considerar variación relevante
#define CAMBIO_ABRUPTO_TEMPERATURA_C    1.0f

// Alias usado por system_logic.c
#define UMBRAL_TEMP_ALERTA_CELSIUS      TEMPERATURA_UMBRAL_ALERTA_C

/* ============================================================
 * CONFIGURACIÓN DE ALERTAS
 * ============================================================ */
/**
 * @brief Límites de acumulación de alertas del sistema.
 *
 * Estos valores indican cuántas alertas amarillas o rojas pueden
 * acumularse antes de considerar una condición crítica o una posible
 * crisis según la lógica general de monitoreo.
 */

#define MAX_ALERTAS_AMARILLAS           5
#define MAX_ALERTAS_ROJAS               3

/* ============================================================
 * CONFIGURACIÓN GENERAL
 * ============================================================ */
/**
 * @brief Parámetros auxiliares de uso general.
 *
 * Incluye configuraciones comunes que pueden ser utilizadas en
 * varios módulos, como el nivel lógico activo del botón y el tamaño
 * máximo de buffers genéricos.
 */

// 0 = botón activo en bajo
#define BUTTON_ACTIVE_LEVEL             0

// Tamaño máximo de buffers genéricos
#define MAX_MUESTRAS_BUFFER             50

/* ============================================================
 * CONFIGURACIÓN MAX30100
 * ============================================================ */
/**
 * @brief Parámetros de adquisición y procesamiento del MAX30100.
 *
 * En esta sección se define la frecuencia de muestreo asumida por
 * el procesamiento, el número de muestras por ventana en modo normal
 * y en alerta, el tamaño del buffer interno y el retardo entre lecturas.
 *
 * Estos valores afectan directamente la calidad del cálculo de BPM,
 * la estabilidad de la señal y el consumo de tiempo en cada ciclo
 * de monitoreo.
 */

// Frecuencia de muestreo usada por el procesamiento
#define MAX30100_SAMPLE_RATE_HZ         50

// Número de muestras por ventana
#define MUESTRAS_MAX30100_NORMAL        100
#define MUESTRAS_MAX30100_ALERTA        200

// Buffer interno del procesamiento
#define MAX30100_BUFFER_SIZE            200

// Delay entre lecturas
#define DELAY_MUESTRAS_MAX30100_MS      10

/* ============================================================
 * CONFIGURACIÓN MLX90614
 * ============================================================ */
/**
 * @brief Parámetros de adquisición del sensor MLX90614.
 *
 * Este bloque define cuántas muestras de temperatura se toman en
 * modo normal y en alerta, el tiempo entre lecturas y los registros
 * internos del sensor usados para temperatura ambiente y temperatura
 * del objeto.
 *
 * Su lógica es proporcionar una base fija para las rutinas de lectura
 * y promediado implementadas en el módulo del sensor.
 */

// Número de muestras por ventana
#define MUESTRAS_TEMP_NORMAL            10
#define MUESTRAS_TEMP_ALERTA            20

// Delay entre lecturas
#define DELAY_MUESTRAS_TEMP_MS          100

// Registros del MLX90614
#define MLX90614_REG_TAMB               0x06
#define MLX90614_REG_TOBJ               0x07

#endif // APP_CONFIG_H