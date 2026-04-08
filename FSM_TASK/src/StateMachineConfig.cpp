/**
 * @file StateMachineConfig.cpp
 * @brief Configuración de la máquina de estados del sistema.
 *
 * @details
 * Este archivo define:
 * - La instancia global de la máquina de estados.
 * - Variables globales del sistema.
 * - Inicialización del sensor DHT.
 * - Configuración de transiciones y callbacks asociados a cada estado.
 */

#include "StateMachineConfig.h"
#include "Tasks.h"

/**
 * @brief Instancia principal de la máquina de estados.
 *
 * @details
 * Se inicializa con:
 * - 6 estados
 * - 13 transiciones posibles
 */
StateMachine stateMachine(6, 13);

/**
 * @brief Entrada actual detectada por el sistema.
 */
Input input;

/**
 * @brief Estado previo del sistema.
 */
State estadoAnterior = INICIO;

/**
 * @brief Estado actual del sistema.
 */
State estadoActual;

/**
 * @brief Valor actual de temperatura leído del sensor DHT.
 */
float temperatura = 0;

/**
 * @brief Valor actual de humedad leído del sensor DHT.
 */
float humedad = 0;

/**
 * @brief Valor actual de luminosidad leído desde el sensor de luz.
 */
float luz = 0;

/**
 * @brief Contador de intentos en estado de alerta.
 */
int intentosAlerta = 0;

/**
 * @brief Estado lógico del LED indicador.
 */
bool ledState = false;

/**
 * @brief Bandera que indica si hubo alerta durante el ciclo actual.
 */
bool huboAlertaEnEsteCiclo = false;

/**
 * @brief Instancia del sensor de temperatura y humedad DHT.
 */
DHT dht(DHTPIN, DHTTYPE);

/**
 * @brief Configura las transiciones y callbacks de la máquina de estados.
 *
 * @details
 * Define:
 * - Transiciones condicionales entre estados.
 * - Funciones ejecutadas al entrar en cada estado.
 *
 * Estados configurados:
 * - INICIO
 * - MONTEMP
 * - MONHUM
 * - MONLUZ
 * - ALERTA
 * - ALARMA
 */
void setupStateMachine()
{
  stateMachine.AddTransition(INICIO, MONTEMP, [](){ return digitalRead(BTN1) == HIGH; });

  stateMachine.AddTransition(MONTEMP, MONLUZ, [](){
    return (digitalRead(SensorGas) == HIGH && input == TIMEOUT2s);
  });

  stateMachine.AddTransition(MONTEMP, MONHUM, [](){ return input == TIMEOUT; });

  stateMachine.AddTransition(MONTEMP, ALERTA, [](){
    return (temperatura > 25 && input == TIMEOUT2s);
  });

  stateMachine.AddTransition(MONHUM, ALERTA, [](){
    return (humedad < 60 && input == TIMEOUT);
  });

  stateMachine.AddTransition(MONHUM, MONTEMP, [](){ return input == TIMEOUT; });

  stateMachine.AddTransition(MONLUZ, ALERTA, [](){
    return (luz < 60 && input == TIMEOUT);
  });

  stateMachine.AddTransition(MONLUZ, MONTEMP, [](){ return input == TIMEOUT; });

  stateMachine.AddTransition(ALERTA, ALARMA, [](){
    return (intentosAlerta >= 3 && temperatura > 30);
  });

  stateMachine.AddTransition(ALERTA, MONTEMP, [](){
    return (estadoAnterior == MONTEMP && intentosAlerta <= 3 && input == TIMEOUT);
  });

  stateMachine.AddTransition(ALERTA, MONHUM, [](){
    return (estadoAnterior == MONHUM && intentosAlerta <= 3 && input == TIMEOUT);
  });

  stateMachine.AddTransition(ALERTA, MONLUZ, [](){
    return (estadoAnterior == MONLUZ && intentosAlerta <= 3 && input == TIMEOUT);
  });

  stateMachine.AddTransition(ALARMA, INICIO, [](){
    return digitalRead(BTN1) == HIGH;
  });

  stateMachine.SetOnEntering(INICIO, onInicio);
  stateMachine.SetOnEntering(MONTEMP, onMontemp);
  stateMachine.SetOnEntering(MONHUM, onMonhum);
  stateMachine.SetOnEntering(MONLUZ, onMonluz);
  stateMachine.SetOnEntering(ALERTA, onAlerta);
  stateMachine.SetOnEntering(ALARMA, onAlarma);
}