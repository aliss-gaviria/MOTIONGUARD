/**
 * @file config.h
 * @brief Parámetros de configuración del nodo sensor basado en ESP32.
 *
 * Este archivo centraliza todos los parámetros configurables del sistema,
 * incluyendo:
 *
 * - Número de muestras a adquirir
 * - Intervalo de muestreo
 * - Parámetros de calibración del sensor MPU6050
 * - Configuración del bus I2C
 * - Sensibilidad del acelerómetro y giroscopio
 * - Configuración del LED indicador
 * - Tiempo de Deep Sleep
 * - Dirección MAC del nodo receptor
 *
 * Separar estos parámetros en un archivo dedicado facilita la
 * modificación y mantenimiento del sistema sin necesidad de
 * alterar la lógica del programa.
 */

#pragma once


/**
 * @brief Número total de muestras que el nodo adquiere antes de dormir.
 */
#define SAMPLE_COUNT          5

/**
 * @brief Intervalo entre muestras en milisegundos.
 */
#define SAMPLE_INTERVAL_MS    18000


/**
 * @brief Número de muestras utilizadas para calibrar el sensor MPU6050.
 */
#define CALIB_SAMPLES         200

/**
 * @brief Retardo entre muestras durante el proceso de calibración (ms).
 */
#define CALIB_DELAY_MS        5


/**
 * @brief Pin GPIO utilizado como línea SCL del bus I2C.
 */
#define I2C_SCL_IO            22

/**
 * @brief Pin GPIO utilizado como línea SDA del bus I2C.
 */
#define I2C_SDA_IO            21

/**
 * @brief Frecuencia de operación del bus I2C.
 */
#define I2C_FREQ_HZ           100000

/**
 * @brief Tiempo máximo de espera en transacciones I2C (ms).
 */
#define I2C_TIMEOUT_MS        100


/**
 * @brief Dirección I2C del sensor MPU6050.
 */
#define MPU6050_ADDR          0x68


/**
 * @brief Sensibilidad del acelerómetro en configuración ±2g.
 *
 * Valor tomado del datasheet del MPU6050.
 * Permite convertir los valores crudos del sensor a unidades de g.
 */
#define ACCEL_SENSITIVITY     16384.0f


/**
 * @brief Sensibilidad del giroscopio en configuración ±250°/s.
 *
 * Permite convertir los valores crudos del sensor a grados por segundo.
 */
#define GYRO_SENSITIVITY      131.0f


/**
 * @brief Pin GPIO utilizado para el LED indicador del nodo.
 */
#define LED_PIN               2


/**
 * @brief Tiempo de permanencia en Deep Sleep (segundos).
 *
 * Una vez enviadas todas las muestras, el ESP32 entra en modo
 * Deep Sleep durante este periodo para reducir el consumo energético.
 */
#define DEEP_SLEEP_SEC        30ULL


/**
 * @brief Dirección MAC del nodo receptor para comunicación ESP-NOW.
 *
 * Esta dirección identifica el dispositivo que recibirá los datos
 * transmitidos por el nodo sensor.
 */
#define RECEIVER_MAC          { 0xB4, 0x3A, 0x45, 0x29, 0xA0, 0x78 }