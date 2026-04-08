/**
 * @file estados.h
 * @brief Definición de los estados de la máquina de estados del sistema.
 *
 * Este archivo declara el tipo enumerado que representa los distintos
 * modos de funcionamiento del sistema de monitoreo biomédico.
 *
 * Su propósito es proporcionar una forma clara y ordenada de controlar
 * la lógica de transición entre las etapas principales del programa.
 */

#ifndef ESTADOS_H
#define ESTADOS_H

/**
 * @brief Enumeración de estados del sistema.
 *
 * Esta enumeración define los estados principales de operación:
 * - estado de espera o inicio
 * - estado de monitoreo de ritmo cardíaco
 * - estado de monitoreo de temperatura
 *
 * La lógica del programa usa estos valores para decidir qué tarea
 * ejecutar en cada momento y cómo pasar de un estado a otro.
 */
typedef enum {
    ESTADO_INICIO = 0,              /**< Estado inicial del sistema, en espera de interacción */
    ESTADO_MONITOREO_RITMO,         /**< Estado dedicado al monitoreo del ritmo cardíaco */
    ESTADO_MONITOREO_TEMPERATURA    /**< Estado dedicado al monitoreo de temperatura */
} estado_sistema_t;

#endif