/**
 * @file config.h
 * @brief Definiciones y variables globales compartidas del sistema.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/queue.h"

/**
 * @brief Pin de entrada para el sensor de pulso.
 */
#define PULSE_PIN GPIO_NUM_34

/**
 * @brief Pin de salida para el LED indicador.
 */
#define LED_PIN   GPIO_NUM_13

/**
 * @brief Canal ADC utilizado.
 */
#define ADC_CANAL ADC_CHANNEL_7

/**
 * @brief Resistencia superior del divisor de voltaje.
 */
#define R1 10000.0

/**
 * @brief Resistencia inferior del divisor de voltaje.
 */
#define R2 10000.0

extern const float CALIBRACION_V;
extern const float VMAX;
extern const float VMIN;

extern float voltajePromedio;
extern float porcentajeBateria;
extern float sumaVoltaje;
extern int contadorMuestras;
extern const int numMuestras;

extern float alpha;
extern int threshold;
extern int hysteresis;

extern float filtered;
extern bool lastState;

extern unsigned long lastBeat;
extern float bpm;

/**
 * @brief Tamaño del buffer de promediado de BPM.
 */
#define RATE_SIZE 16

extern float rates[RATE_SIZE];
extern int rateSpot;
extern float bpmAvg;

extern bool fingerDetected;

extern QueueHandle_t bpmQueue;

/**
 * @brief Estados de la máquina de estados del sistema.
 */
typedef enum {
    INIT,          /**< Estado inicial */
    STABILIZING,   /**< Estabilización de señal */
    MONITORING,    /**< Monitoreo activo */
    SLEEPING       /**< Modo de bajo consumo */
} State;

extern State currentState;

extern bool stabilizationFinished;
extern bool monitoringFinished;

extern uint8_t receiverMac[6];

/**
 * @brief Tiempo de estabilización en milisegundos.
 */
#define STABILIZATION_TIME 15000

/**
 * @brief Tiempo de monitoreo en milisegundos.
 */
#define MONITORING_TIME    10000

extern int64_t stateStartTime;

#endif