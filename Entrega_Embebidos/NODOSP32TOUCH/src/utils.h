/**
 * @file utils.h
 * @brief Funciones auxiliares del sistema
 */

/**
 * @file utils.h
 * @brief Funciones auxiliares del sistema
 */

#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

/**
 * @brief Obtiene la fecha y hora actual del sistema
 * @param buffer Buffer donde se almacena la fecha
 * @param len Tamaño del buffer
 * @return true si se obtuvo correctamente
 */
bool obtenerFechaHora(char *buffer, size_t len);

#endif