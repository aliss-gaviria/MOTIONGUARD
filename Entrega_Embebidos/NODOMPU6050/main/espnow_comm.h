/**
 * @file espnow_comm.h
 * @brief Interfaz del módulo de comunicación ESP-NOW.
 *
 * Este archivo define las funciones públicas utilizadas para inicializar
 * la comunicación inalámbrica y transmitir datos provenientes del
 * sensor MPU6050.
 */

#pragma once
#include "mpu6050.h"

/**
 * @brief Inicializa el sistema de comunicación ESP-NOW.
 *
 * Configura el módulo WiFi del ESP32 en modo estación (STA) e
 * inicializa el protocolo ESP-NOW. Además, registra el nodo
 * receptor como peer autorizado utilizando la dirección MAC
 * definida en `config.h`.
 */
void espnow_init(void);

/**
 * @brief Envía los datos del sensor mediante ESP-NOW.
 *
 * Esta función serializa las mediciones del sensor MPU6050
 * en formato CSV y las transmite al nodo receptor.
 *
 * @param data Puntero a la estructura que contiene las mediciones
 *             del sensor.
 */
void send_data_espnow(const mpu_values_t *data);