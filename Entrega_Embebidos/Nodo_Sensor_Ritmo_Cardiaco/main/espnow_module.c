/**
 * @file espnow_module.c
 * @brief Inicialización y gestión de comunicación mediante ESP-NOW.
 */

#include <string.h>
#include <stdlib.h>

#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "cJSON.h"

#include "config.h"
#include "espnow_module.h"

/**
 * @brief Inicializa el sistema WiFi en modo estación y configura ESP-NOW.
 * 
 * Configura la interfaz de red, inicializa WiFi y registra el peer
 * con la dirección MAC definida.
 */
void init_esp_now(void) {

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    esp_now_init();

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, receiverMac, 6);
    peer.ifidx = WIFI_IF_STA;
    peer.channel = 0;
    peer.encrypt = false;

    esp_now_add_peer(&peer);
}

/**
 * @brief Envía datos mediante ESP-NOW en formato JSON.
 * 
 * Construye un arreglo JSON con:
 * - Identificador del nodo
 * - BPM
 * - Voltaje promedio
 * - Porcentaje de batería
 * 
 * @param bpmValue Valor de BPM a transmitir.
 */
void sendData(float bpmValue) {
    
    cJSON *doc = cJSON_CreateArray();

    cJSON_AddItemToArray(doc, cJSON_CreateNumber(3));
    cJSON_AddItemToArray(doc, cJSON_CreateNumber(bpmValue));
    cJSON_AddItemToArray(doc, cJSON_CreateNumber(voltajePromedio));
    cJSON_AddItemToArray(doc, cJSON_CreateNumber(porcentajeBateria));

    char *buffer = cJSON_PrintUnformatted(doc);

    esp_now_send(receiverMac, (uint8_t *)buffer, strlen(buffer));

    cJSON_Delete(doc);
    free(buffer);
}