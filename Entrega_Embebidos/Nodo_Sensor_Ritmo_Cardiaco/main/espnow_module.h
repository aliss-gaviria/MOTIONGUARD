/**
 * @file espnow_module.h
 * @brief Interfaz para la comunicación inalámbrica mediante ESP-NOW.
 */

#ifndef ESPNOW_MODULE_H
#define ESPNOW_MODULE_H

/**
 * @brief Inicializa el sistema ESP-NOW.
 * 
 * Configura la interfaz WiFi en modo estación y prepara la comunicación
 * con el nodo receptor.
 */
void init_esp_now(void);

/**
 * @brief Envía datos a través de ESP-NOW.
 * 
 * @param bpmValue Valor de BPM a transmitir.
 */
void sendData(float bpmValue);

#endif