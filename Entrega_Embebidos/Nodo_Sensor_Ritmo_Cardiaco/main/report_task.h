/**
 * @file report_task.h
 * @brief Interfaz de la tarea de envío de datos del sistema.
 */

#ifndef REPORT_TASK_H
#define REPORT_TASK_H

/**
 * @brief Tarea encargada del envío de datos de BPM.
 * 
 * Lee los valores desde la cola y los transmite mediante ESP-NOW.
 * 
 * @param pv Parámetro no utilizado.
 */
void task_report(void *pv);

#endif