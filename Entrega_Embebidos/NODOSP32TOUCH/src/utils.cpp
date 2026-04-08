/**
 * @file utils.cpp
 * @brief Funciones auxiliares del sistema relacionadas con tiempo.
 *
 * @details
 * Este módulo permite:
 * - Obtener fecha y hora desde el sistema (NTP)
 * - Formatear la salida en texto legible
 *
 * Dependencias:
 * - WiFi debe estar conectado
 * - configTime() debe haberse ejecutado previamente
 */

#include "utils.h"
#include "time.h"

/**
 * @brief Obtiene la fecha y hora actual del sistema.
 *
 * @param buffer Buffer donde se almacenará la cadena formateada.
 * @param len Tamaño del buffer.
 *
 * @return true si se obtuvo correctamente, false si falla.
 *
 * @note
 * Usa la configuración NTP previamente establecida.
 *
 * @warning
 * Si no hay conexión WiFi o NTP, retornará false.
 */
bool obtenerFechaHora(char *buffer, size_t len)
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return false;
    strftime(buffer, len, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return true;
}