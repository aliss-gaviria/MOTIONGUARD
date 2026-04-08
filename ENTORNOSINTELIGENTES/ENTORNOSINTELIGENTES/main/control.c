/**
 * @file control.c
 * @brief Implementación de las funciones de control de entradas y salidas digitales.
 *
 * Este módulo se encarga de manejar los periféricos digitales básicos del sistema:
 * - un LED indicador
 * - un botón de usuario
 *
 * Su función principal es abstraer la configuración y el uso de estos GPIO,
 * de forma que el resto del programa pueda encender, apagar o leer el botón
 * sin preocuparse por los detalles de bajo nivel.
 */

#include "control.h"
#include "app_config.h"

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Etiqueta usada para mensajes de depuración del módulo.
 *
 * Se utiliza en los mensajes de log para identificar que los eventos
 * reportados pertenecen al módulo de control de entradas y salidas.
 */
static const char *TAG_IO = "IO_CONTROL";

/* ============================================================
 * FUNCIONES DE INICIALIZACIÓN
 * ============================================================ */

/**
 * @brief Inicializa el GPIO asociado al LED como salida digital.
 *
 * Esta función configura el pin definido por `LED_GPIO` como salida,
 * desactiva resistencias internas pull-up y pull-down, y deja el LED
 * apagado al iniciar.
 *
 * Lógica de funcionamiento:
 * - Se crea una estructura `gpio_config_t` con la configuración deseada.
 * - Se aplica la configuración al pin del LED.
 * - Se fuerza el nivel lógico bajo para comenzar con el LED apagado.
 * - Se registra un mensaje informativo por el monitor de logs.
 */
void led_inicializar(void)
{
    gpio_config_t config_led = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&config_led);
    gpio_set_level(LED_GPIO, 0);

    ESP_LOGI(TAG_IO, "LED inicializado en GPIO%d", LED_GPIO);
}

/**
 * @brief Inicializa el GPIO asociado al botón como entrada digital.
 *
 * Esta función configura el pin definido por `BUTTON_GPIO` como entrada,
 * habilitando la resistencia pull-up interna del microcontrolador.
 *
 * Lógica de funcionamiento:
 * - El botón queda normalmente en nivel alto gracias al pull-up interno.
 * - Cuando el botón se presiona, el pin pasa a nivel bajo.
 * - Esta configuración permite una lectura estable sin requerir
 *   una resistencia pull-up externa.
 *
 * Además, la interrupción queda deshabilitada porque en este diseño
 * el botón se consulta por sondeo (polling).
 */
void boton_inicializar(void)
{
    gpio_config_t config_boton = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&config_boton);

    ESP_LOGI(TAG_IO, "Botón inicializado en GPIO%d", BUTTON_GPIO);
}

/* ============================================================
 * FUNCIONES DE CONTROL DEL LED
 * ============================================================ */

/**
 * @brief Enciende el LED del sistema.
 *
 * Esta función coloca en nivel lógico alto el GPIO del LED,
 * produciendo su encendido.
 *
 * Su propósito es ofrecer una interfaz simple para indicar estados
 * del sistema sin exponer directamente el manejo del GPIO.
 */
void led_encender(void)
{
    gpio_set_level(LED_GPIO, 1);
}

/**
 * @brief Apaga el LED del sistema.
 *
 * Esta función coloca en nivel lógico bajo el GPIO del LED,
 * produciendo su apagado.
 *
 * Se usa para indicar estados inactivos, de espera o finalización
 * de ciertas acciones dentro del sistema.
 */
void led_apagar(void)
{
    gpio_set_level(LED_GPIO, 0);
}

/* ============================================================
 * FUNCIÓN DE LECTURA DEL BOTÓN
 * ============================================================ */

/**
 * @brief Verifica si el botón ha sido presionado.
 *
 * @return true si el botón está presionado de forma válida.
 * @return false si el botón no está presionado.
 *
 * Lógica de funcionamiento:
 * - Primero se lee el estado del GPIO del botón.
 * - Si se detecta nivel bajo, se espera un pequeño tiempo de 20 ms.
 * - Luego se realiza una segunda lectura para confirmar la pulsación.
 *
 * Esta técnica actúa como un anti-rebote básico por software
 * (software debounce), evitando que una sola pulsación física
 * genere múltiples detecciones falsas debido al rebote mecánico
 * del botón.
 */
bool boton_esta_presionado(void)
{
    if (gpio_get_level(BUTTON_GPIO) == 0) {
        vTaskDelay(pdMS_TO_TICKS(20));
        if (gpio_get_level(BUTTON_GPIO) == 0) {
            return true;
        }
    }
    return false;
}