
/**
 * @file sdcard.cpp
 * @brief Manejo de almacenamiento persistente en tarjeta SD.
 *
 * @details
 * Este módulo permite:
 * - Guardar datos en archivo de texto
 * - Asociar cada registro con timestamp
 * - Crear logs estructurados del sistema
 *
 * Archivo utilizado:
 * /datos.txt
 */
#include "sd_card.h"
#include "config.h"
#include "utils.h"

/**
 * @brief Guarda un dato en la tarjeta SD con timestamp.
 *
 * @param texto Cadena de texto a almacenar.
 *
 * @note
 * El formato final será:
 * [YYYY-MM-DD HH:MM:SS] TEXTO
 *
 * @warning
 * Si la SD falla o no está inicializada, no se guardará nada.
 */
void guardarDatoIndividual(const char *texto)
{
    char fechaHora[32];

    if (!obtenerFechaHora(fechaHora, sizeof(fechaHora))) {
        snprintf(fechaHora, sizeof(fechaHora), "T:%lu", millis());
    }

    char fullBuffer[320];
    snprintf(fullBuffer, sizeof(fullBuffer), "[%s] %s", fechaHora, texto);

    File file = SD.open("/datos.txt", FILE_APPEND);

    if (file) {
        file.println(fullBuffer);
        file.close();
        Serial.println("Guardado OK");
    } else {
        Serial.println("Error al abrir archivo");
    }
}