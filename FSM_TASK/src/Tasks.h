/**
 * @file Tasks.h
 * @brief Declaración de tareas asincrónicas y funciones asociadas.
 *
 * @details
 * Contiene:
 * - Declaraciones externas de objetos AsyncTask.
 * - Prototipos de funciones utilizadas como callbacks de tareas.
 */

#ifndef TASKS_H
#define TASKS_H

#include "AsyncTaskLib.h"

/**
 * @brief Tareas asincrónicas utilizadas por el sistema.
 */
extern AsyncTask TaskTimeout;
extern AsyncTask TaskTimeout2s;
extern AsyncTask TaskTemp;
extern AsyncTask TaskHum;
extern AsyncTask TaskLuz;
extern AsyncTask TaskPrint;
extern AsyncTask TaskLed;

/**
 * @brief Callbacks asociados a temporizadores.
 */
void functTimeout();
void functTimeout2s();

/**
 * @brief Funciones de lectura de sensores.
 */
void readTemperatura();
void readHumedad();
void readLuz();

/**
 * @brief Funciones auxiliares del sistema.
 */
void functPrint();
void funcLedRGB();

#endif