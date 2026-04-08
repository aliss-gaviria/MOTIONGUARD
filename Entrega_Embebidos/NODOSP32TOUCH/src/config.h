/**
 * @file config.h
 * @brief Definiciones globales, constantes y variables compartidas del sistema.
 *
 * @details
 * Este archivo centraliza:
 * - Configuración de pines (SD, SPI, etc.)
 * - Constantes físicas de sensores (escalas)
 * - Variables globales compartidas entre tareas
 * - Recursos del sistema (memoria gráfica, mutex)
 *
 * Permite desacoplar módulos y evitar duplicación de código.
 *
 * @note
 * Todas las variables declaradas aquí deben ser definidas en main.cpp
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <SPI.h>
#include "LCD_Test.h"
#include "SD.h"

/**
 * @brief Pines de conexión para la tarjeta SD (SPI)
 */

#define SD_CS_PIN 17
#define SD_CLK_PIN 18
#define SD_MOSI_PIN 21
#define SD_MISO_PIN 33

extern SPIClass sdSPI;

/**
 * @brief Factor de conversión del acelerómetro
 */
#define ACC_SCALE 16384.0

/**
 * @brief Factor de conversión del giroscopio
 */
#define GYRO_SCALE 16.4

/**
 * @brief Tamaño del buffer de imagen en la LCD
 */
extern uint32_t Imagesize;

/**
 * @brief Puntero a memoria dinámica usada como framebuffer
 */
extern UWORD *BlackImage;

/**
 * @brief Arreglos globales para almacenar datos del sensor IMU
 */
extern float acc[3], gyro[3];

/**
 * @brief Nivel de batería recibido desde nodo remoto
 */
extern float bateria;

/**
 * @brief Contador de tiempo interno del sensor
 */
extern unsigned int tim_count;

/**
 * @brief Intervalo de muestreo del sensor en milisegundos
 */
#define INTERVALO_MUESTREO 20000

/**
 * @brief Mutex para proteger acceso a variables compartidas entre tareas
 */
extern SemaphoreHandle_t xDisplayMutex;

#endif