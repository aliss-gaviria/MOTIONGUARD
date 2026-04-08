/**
 * @file sdcard.h
 * @brief Manejo de almacenamiento en tarjeta SD
 */

#ifndef SD_CARD_H
#define SD_CARD_H

#include <Arduino.h>

/**
 * @brief Guarda un dato en la SD con timestamp
 * @param texto Texto a guardar
 */
void guardarDatoIndividual(const char *texto);

#endif