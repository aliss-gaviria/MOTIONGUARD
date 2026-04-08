/**
 * @file main.cpp
 * @brief Archivo principal del sistema.
 */

#include "StateMachineConfig.h"
#include "Tasks.h"

/**
 * @brief Inicializa el sistema.
 */
void setup()
{
  Serial.begin(115200);
  pinMode(SensorGas, INPUT);
  pinMode(PinLuz, INPUT);
  pinMode(LedRojo, OUTPUT);
  pinMode(LedVerde, OUTPUT);
  pinMode(LedAzul, OUTPUT);
  pinMode(BTN1, INPUT);

  dht.begin();
  setupStateMachine();
  stateMachine.SetState(INICIO, false, true);
}

/**
 * @brief Ejecuta el ciclo principal del sistema.
 */
void loop()
{
  input = static_cast<Input>(readInput());

  TaskTemp.Update();
  TaskHum.Update();
  TaskLuz.Update();
  TaskPrint.Update();
  TaskLed.Update();
  TaskTimeout.Update();
  TaskTimeout2s.Update();

  stateMachine.Update();

  input = Unknown;
}