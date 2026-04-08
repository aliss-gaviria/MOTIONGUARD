/**
 * @file StateMachineConfig.h
 * @brief Declaraciones y configuración de la máquina de estados del sistema.
 *
 * @details
 * Contiene:
 * - Definición de pines del hardware.
 * - Enumeraciones de estados y eventos.
 * - Declaración de variables globales compartidas.
 * - Prototipos de funciones asociadas a la máquina de estados.
 */

#ifndef STATEMACHINECONFIG_H
#define STATEMACHINECONFIG_H

#include "StateMachineLib.h"
#include "AsyncTaskLib.h"
#include "DHT.h"


#define DHTPIN 27
#define DHTTYPE DHT22
#define LedRojo 33
#define LedVerde 25
#define LedAzul 26
#define PinLuz 32
#define SensorGas 14
#define BTN1 12

/**
 * @enum State
 * @brief Estados posibles del sistema.
 */
enum State
{
  INICIO,
  MONTEMP,
  MONHUM,
  MONLUZ,
  ALERTA,
  ALARMA
};

/**
 * @enum Input
 * @brief Eventos que generan transiciones en la máquina de estados.
 */
enum Input
{
  Reset,
  Forward,
  Backward,
  Unknown,
  TIMEOUT,
  TIMEOUT2s
};


extern StateMachine stateMachine;
extern Input input;
extern State estadoAnterior;
extern State estadoActual;

extern float temperatura;
extern float humedad;
extern float luz;
extern int intentosAlerta;
extern bool ledState;
extern bool huboAlertaEnEsteCiclo;

extern DHT dht;

/**
 * @brief Configura transiciones y callbacks de la máquina de estados.
 */
void setupStateMachine();

/**
 * @brief Lee la entrada actual del sistema.
 * @return Código entero correspondiente al evento detectado.
 */
int readInput();


void onInicio();
void onMontemp();
void onMonhum();
void onMonluz();
void onAlerta();
void onAlarma();

/**
 * @brief Detiene todas las tareas activas.
 */
void stopAllTasks();

#endif