/**
 * @file sensor_task.h
 * @brief Interfaz de la tarea de adquisición y procesamiento del sensor de pulso.
 */

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

/**
 * @brief Tarea encargada del procesamiento de la señal del sensor.
 * 
 * Realiza la lectura del ADC, procesamiento de la señal y cálculo
 * del ritmo cardíaco en BPM.
 * 
 * @param pv Parámetro no utilizado.
 */
void task_sensor(void *pv);

#endif