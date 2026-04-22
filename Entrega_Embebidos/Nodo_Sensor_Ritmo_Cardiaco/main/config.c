/**
 * @file config.c
 * @brief Definición de variables globales del sistema.
 */

#include "config.h"

/**
 * @brief Factor de calibración para la medición de voltaje.
 */
const float CALIBRACION_V = 1.04;

/**
 * @brief Voltaje máximo de la batería.
 */
const float VMAX = 4.2;

/**
 * @brief Voltaje mínimo de la batería.
 */
const float VMIN = 3.0;

/**
 * @brief Voltaje promedio calculado.
 */
float voltajePromedio = 0;

/**
 * @brief Porcentaje de batería calculado.
 */
float porcentajeBateria = 0;

/**
 * @brief Acumulador de voltaje para promedio.
 */
float sumaVoltaje = 0;

/**
 * @brief Contador de muestras de voltaje.
 */
int contadorMuestras = 0;

/**
 * @brief Número de muestras para el cálculo del promedio.
 */
const int numMuestras = 25;

/**
 * @brief Coeficiente del filtro exponencial.
 */
float alpha = 0.2;

/**
 * @brief Umbral de detección de señal.
 */
int threshold = 2400;

/**
 * @brief Histéresis aplicada al umbral.
 */
int hysteresis = 50;

/**
 * @brief Señal filtrada.
 */
float filtered = 0;

/**
 * @brief Estado anterior de la señal.
 */
bool lastState = false;

/**
 * @brief Tiempo del último latido detectado.
 */
unsigned long lastBeat = 0;

/**
 * @brief Valor instantáneo de BPM.
 */
float bpm = 0;

/**
 * @brief Buffer de valores de BPM para promedio.
 */
float rates[RATE_SIZE] = {0};

/**
 * @brief Índice actual del buffer de BPM.
 */
int rateSpot = 0;

/**
 * @brief Valor promedio de BPM.
 */
float bpmAvg = 0;

/**
 * @brief Indica si se detecta el dedo en el sensor.
 */
bool fingerDetected = false;

/**
 * @brief Cola para almacenamiento de datos BPM.
 */
QueueHandle_t bpmQueue = NULL;

/**
 * @brief Estado actual de la máquina de estados.
 */
State currentState = INIT;

/**
 * @brief Indica si la fase de estabilización ha finalizado.
 */
bool stabilizationFinished = false;

/**
 * @brief Indica si la fase de monitoreo ha finalizado.
 */
bool monitoringFinished = false;

/**
 * @brief Dirección MAC del nodo receptor.
 */
uint8_t receiverMac[6] = {0xB4, 0x3A, 0x45, 0x29, 0xA0, 0x78};

/**
 * @brief Marca de tiempo de inicio del estado actual.
 */
int64_t stateStartTime = 0;