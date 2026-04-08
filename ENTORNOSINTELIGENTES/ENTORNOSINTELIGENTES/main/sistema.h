/**
 * @file sistema.h
 * @brief Declaraciones públicas de la lógica principal del sistema de monitoreo.
 *
 * Este archivo define la interfaz pública del módulo encargado de la
 * lógica de decisión del sistema, incluyendo:
 * - evaluación de alertas rojas por ritmo cardíaco
 * - evaluación de alertas amarillas por temperatura
 * - verificación de posible crisis
 * - ejecución de la máquina de estados principal
 *
 * Su propósito es permitir que el resto del programa invoque la lógica
 * general del sistema sin depender de los detalles de implementación.
 */

#ifndef SISTEMA_H
#define SISTEMA_H

#include <stdbool.h>
#include "app_types.h"

/* ============================================================
 * FUNCIONES PÚBLICAS DEL MÓDULO DE SISTEMA
 * ============================================================ */

/**
 * @brief Evalúa si un resultado de ritmo cardíaco activa una alerta roja.
 *
 * @param resultado Puntero a la estructura con el resultado del monitoreo de ritmo.
 * @param ctx Puntero al contexto global del sistema.
 * @return true si se detecta una alerta roja.
 * @return false si no se activa alerta roja.
 *
 * Esta función compara el BPM calculado con los límites definidos como
 * normales y actualiza el contexto si la condición de alerta se cumple.
 */
bool evaluar_alerta_roja(const ResultadoRitmo_t *resultado, ContextoSistema_t *ctx);

/**
 * @brief Evalúa si un resultado de temperatura activa una alerta amarilla.
 *
 * @param resultado Puntero a la estructura con el resultado del monitoreo térmico.
 * @param ctx Puntero al contexto global del sistema.
 * @return true si se detecta una alerta amarilla.
 * @return false si no se activa alerta amarilla.
 *
 * Esta función analiza la temperatura medida y su variación respecto a
 * la lectura anterior para determinar si existe una condición de alerta.
 */
bool evaluar_alerta_amarilla(const ResultadoTemperatura_t *resultado, ContextoSistema_t *ctx);

/**
 * @brief Verifica si el sistema presenta una posible crisis según las alertas acumuladas.
 *
 * @param ctx Puntero al contexto global del sistema.
 * @return true si se detecta una posible crisis.
 * @return false si no se cumple la condición.
 *
 * Esta función revisa los contadores de alertas amarillas y rojas y
 * determina si se ha alcanzado un nivel crítico acumulado.
 */
bool verificar_posible_crisis(const ContextoSistema_t *ctx);

/**
 * @brief Ejecuta la máquina de estados principal del sistema.
 *
 * @param ctx Puntero al contexto global del sistema.
 *
 * Esta función coordina el flujo general de operación del sistema,
 * alternando entre los distintos estados definidos y llamando a los
 * módulos de monitoreo y evaluación correspondientes.
 */
void ejecutar_maquina_estados(ContextoSistema_t *ctx);

#endif