/**
 * @file state_machine.h
 * @brief Interfaz de la máquina de estados del sistema.
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

/**
 * @brief Tarea que gestiona la máquina de estados.
 * 
 * Controla el flujo de operación del sistema mediante la transición
 * entre estados definidos, coordinando el comportamiento general.
 * 
 * @param pv Parámetro no utilizado.
 */
void state_machine_task(void *pv);

#endif