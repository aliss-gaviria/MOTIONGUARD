/**
 * @file espnow.h
 * @brief Comunicación ESP-NOW
 */

#ifndef WIFI_ESPNOW_H
#define WIFI_ESPNOW_H

#include <Arduino.h>

/**
 * @brief Callback de recepción de datos ESP-NOW
 */
void on_data_recv(const uint8_t *mac, const uint8_t *data, int len);

#endif