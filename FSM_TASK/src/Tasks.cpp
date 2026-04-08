/**
 * @file Tasks.cpp
 * @brief Implementación de tareas asincrónicas y callbacks de estados.
 *
 * @details
 * Contiene:
 * - Definición de tareas periódicas mediante AsyncTask.
 * - Implementación de funciones ejecutadas al entrar en cada estado.
 * - Funciones de lectura de sensores.
 * - Funciones asociadas a temporizadores.
 * - Control del LED RGB.
 */

#include "StateMachineConfig.h"
#include "Tasks.h"

AsyncTask TaskTimeout(2000, false, functTimeout);
AsyncTask TaskTimeout2s(2000, false, functTimeout2s);
AsyncTask TaskTemp(500, true, readTemperatura);
AsyncTask TaskHum(500, true, readHumedad);
AsyncTask TaskLuz(500, true, readLuz);
AsyncTask TaskPrint(500, true, functPrint);
AsyncTask TaskLed(200, true, funcLedRGB);

/**
 * @brief Detiene todas las tareas periódicas activas.
 */
void stopAllTasks()
{
  TaskTemp.Stop();
  TaskHum.Stop();
  TaskLuz.Stop();
  TaskPrint.Stop();
  TaskLed.Stop();
  TaskTimeout2s.Stop();
}


/**
 * @brief Acción ejecutada al entrar en el estado INICIO.
 */
void onInicio()
{
  stopAllTasks();
  TaskTimeout.Stop();
  estadoActual = INICIO;
  intentosAlerta = 0;
  ledState = false;
  TaskLed.SetIntervalMillis(200);
  TaskLed.Start();

  Serial.println("I   MT   MH   ML    Alerta    Alarma");
  Serial.println("X\n");
}

/**
 * @brief Acción ejecutada al entrar en el estado MONTEMP.
 */
void onMontemp()
{
  stopAllTasks();
  if (estadoAnterior != ALERTA && !huboAlertaEnEsteCiclo)
  {
      intentosAlerta = 0;
  }

  huboAlertaEnEsteCiclo = false;
  
  estadoActual = MONTEMP;
  estadoAnterior = MONTEMP;

  TaskTemp.Start();
  TaskPrint.Start();

  TaskTimeout.SetIntervalMillis(4000);
  TaskTimeout.Start();
  TaskTimeout2s.Start();

  TaskLed.Start();

  Serial.println("I   MT   MH   ML    Alerta    Alarma");
  Serial.println("    X\n");
}

/**
 * @brief Acción ejecutada al entrar en el estado MONHUM.
 */
void onMonhum()
{
  stopAllTasks();
  estadoActual = MONHUM;
  estadoAnterior = MONHUM;

  TaskHum.Start();
  TaskPrint.Start();

  TaskTimeout.SetIntervalMillis(3000);
  TaskTimeout.Start();

  TaskLed.Start();

  Serial.println("I   MT   MH   ML    Alerta    Alarma");
  Serial.println("         X\n");
}

/**
 * @brief Acción ejecutada al entrar en el estado MONLUZ.
 */
void onMonluz()
{
  stopAllTasks();
  estadoActual = MONLUZ;
  estadoAnterior = MONLUZ;

  TaskLuz.Start();
  TaskPrint.Start();

  TaskTimeout.SetIntervalMillis(3000);
  TaskTimeout.Start();

  TaskLed.Start();

  Serial.println("I   MT   MH   ML    Alerta    Alarma");
  Serial.println("              X\n");
}

/**
 * @brief Acción ejecutada al entrar en el estado ALERTA.
 */
void onAlerta()
{
  stopAllTasks();
  estadoActual = ALERTA;
  intentosAlerta++;
  huboAlertaEnEsteCiclo = true;

  TaskTimeout.SetIntervalMillis(2000);
  TaskTimeout.Start();

  TaskLed.SetIntervalMillis(500);
  TaskLed.Start();

  Serial.print("Intentos: ");
  Serial.println(intentosAlerta);

  Serial.println("I   MT   MH   ML    Alerta    Alarma");
  Serial.println("                       X\n");
}

/**
 * @brief Acción ejecutada al entrar en el estado ALARMA.
 */
void onAlarma()
{
  intentosAlerta = 0;
  estadoActual = ALARMA;
  TaskTimeout.Stop();

  TaskLed.SetIntervalMillis(100);
  TaskLed.Start();

  Serial.println("I   MT   MH   ML    Alerta    Alarma");
  Serial.println("                                X\n");
}


/**
 * @brief Lee comandos desde el puerto serial.
 * @return Evento detectado como tipo Input.
 */
int readInput()
{
  Input currentInput = Unknown;

  if (Serial.available())
  {
    char incomingChar = Serial.read();

    switch (incomingChar)
    {
      case 'R': currentInput = Reset; break;
      case 'A': currentInput = Backward; break;
      case 'D': currentInput = Forward; break;
    }
  }

  return currentInput;
}


/**
 * @brief Lee la temperatura desde el sensor DHT.
 */
void readTemperatura(){
    boolean GasState;
    GasState = digitalRead(SensorGas);
    temperatura = dht.readTemperature();
    Serial.println(GasState);
    Serial.println(digitalRead(BTN1));
    temperatura = dht.readTemperature();
}

/**
 * @brief Lee la humedad desde el sensor DHT.
 */
void readHumedad(){ humedad = dht.readHumidity(); }

/**
 * @brief Lee el nivel de luz desde el sensor analógico.
 */
void readLuz(){ luz = analogRead(PinLuz)/40.95; }

/**
 * @brief Callback ejecutado al cumplirse el TIMEOUT.
 */
void functTimeout(){ input = TIMEOUT; }

/**
 * @brief Callback ejecutado al cumplirse el TIMEOUT2s.
 */
void functTimeout2s(){ input = TIMEOUT2s; }

/**
 * @brief Imprime en el puerto serial la variable monitoreada según el estado actual.
 */
void functPrint()
{
  if (estadoActual == MONTEMP){
    Serial.print("Temp: "); Serial.println(temperatura);
  }
  else if (estadoActual == MONHUM){
    Serial.print("Humedad: "); Serial.println(humedad);
  }
  else if (estadoActual == MONLUZ){
    Serial.print("Luz: "); Serial.println(luz);
  }
}

/**
 * @brief Controla el LED RGB según el estado actual del sistema.
 */
void funcLedRGB()
{
  ledState = !ledState;

  digitalWrite(LedRojo, HIGH);
  digitalWrite(LedVerde, HIGH);
  digitalWrite(LedAzul, HIGH);

  switch (estadoActual)
  {
    case INICIO: digitalWrite(LedVerde, ledState ? LOW : HIGH); break;
    case ALERTA: digitalWrite(LedAzul, ledState ? LOW : HIGH); break;
    case ALARMA: digitalWrite(LedRojo, ledState ? LOW : HIGH); break;
    default: break;
  }
}