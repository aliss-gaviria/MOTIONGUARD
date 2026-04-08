/**
 * @file main.c
 * @brief Punto de entrada principal del sistema de monitoreo biomédico.
 *
 * Este archivo contiene la función `app_main()`, que es el punto inicial
 * de ejecución en ESP-IDF. Su responsabilidad es preparar todos los
 * módulos del sistema antes de iniciar la operación normal.
 *
 * La lógica general implementada en este archivo es:
 * - mostrar un mensaje de arranque por consola
 * - inicializar los periféricos básicos de entrada y salida
 * - inicializar los buses I2C y sensores
 * - crear el contexto inicial del sistema
 * - arrancar la máquina de estados principal
 *
 * En otras palabras, este módulo actúa como coordinador de arranque
 * del sistema completo.
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "app_config.h"
#include "estados.h"
#include "app_types.h"

#include "control.h"
#include "max30100.h"
#include "mlx90614.h"
#include "sistema.h"

/**
 * @brief Etiqueta de logs del módulo principal.
 *
 * Se utiliza para identificar en consola los mensajes generados
 * desde el archivo principal del sistema.
 */
static const char *TAG_MAIN = "MAIN";

/* ============================================================
 * FUNCIÓN PRINCIPAL DEL SISTEMA
 * ============================================================ */

/**
 * @brief Función principal de ejecución del sistema en ESP-IDF.
 *
 * Esta función es llamada automáticamente al iniciar el microcontrolador
 * y constituye el punto de entrada del programa.
 *
 * @note En ESP-IDF no se usa `main()` tradicional, sino `app_main()`.
 *
 * Lógica de funcionamiento:
 * 1. Muestra un encabezado de arranque en el monitor serial.
 * 2. Inicializa los periféricos GPIO básicos:
 *    - LED
 *    - botón
 * 3. Inicializa el bus I2C del sensor MAX30100.
 * 4. Inicializa y configura el sensor MAX30100.
 * 5. Inicializa el bus I2C del sensor MLX90614.
 * 6. Crea e inicializa el contexto global del sistema.
 * 7. Muestra mensajes de sistema listo.
 * 8. Llama a la máquina de estados principal para comenzar la operación.
 *
 * Manejo de errores:
 * - Si falla la inicialización de alguno de los módulos críticos,
 *   se reporta el error por consola y la función termina sin continuar.
 *
 * Observación:
 * - Una vez se llama `ejecutar_maquina_estados()`, el control normal
 *   del programa pasa a la lógica del sistema y en teoría no debería
 *   regresar a esta función.
 */
void app_main(void)
{
    /**
     * @brief Variable auxiliar para verificar el resultado de cada
     * etapa de inicialización.
     *
     * Se reutiliza para almacenar el código de retorno de las funciones
     * críticas del sistema y actuar en caso de error.
     */
    esp_err_t resultado_inicializacion;

    printf("\n");
    printf("=============================================\n");
    printf("   SISTEMA DE MONITOREO BIOMEDICO - ESP32S3  \n");
    printf("=============================================\n");
    printf("Inicializando sistema...\n");

    /* =========================================================
     * 1. INICIALIZAR GPIO
     * ========================================================= */
    led_inicializar();
    boton_inicializar();

    /* =========================================================
     * 2. INICIALIZAR I2C DEL MAX30100
     * ========================================================= */
    resultado_inicializacion = i2c_inicializar_max30100();
    if (resultado_inicializacion != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Error al inicializar I2C del MAX30100: %s",
                 esp_err_to_name(resultado_inicializacion));
        printf("FALLO CRITICO: no se pudo inicializar I2C del MAX30100\n");
        return;
    }

    /* =========================================================
     * 3. INICIALIZAR MAX30100
     * ========================================================= */
    resultado_inicializacion = max30100_inicializar();
    if (resultado_inicializacion != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Error al inicializar MAX30100: %s",
                 esp_err_to_name(resultado_inicializacion));
        printf("FALLO CRITICO: no se pudo inicializar el sensor MAX30100\n");
        return;
    }

    /* =========================================================
     * 4. INICIALIZAR I2C DEL MLX90614
     * ========================================================= */
    resultado_inicializacion = i2c_inicializar_mlx90614();
    if (resultado_inicializacion != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "Error al inicializar I2C del MLX90614: %s",
                 esp_err_to_name(resultado_inicializacion));
        printf("FALLO CRITICO: no se pudo inicializar I2C del MLX90614\n");
        return;
    }

    /* =========================================================
     * 5. CREAR CONTEXTO DEL SISTEMA
     * ========================================================= */
    /**
     * @brief Contexto principal del sistema.
     *
     * Esta estructura almacena el estado actual de la máquina de estados
     * y la información acumulada necesaria para la lógica del sistema,
     * como contadores de alertas y temperatura anterior.
     *
     * Se inicializa en el estado de inicio con todos los contadores en cero.
     */
    ContextoSistema_t contexto_sistema = {
        .estado_actual = ESTADO_INICIO,
        .contador_alertas_amarillas = 0,
        .contador_alertas_rojas = 0,
        .temperatura_anterior_celsius = 0.0f
    };

    printf("Inicializacion completada correctamente.\n");
    printf("Sistema en estado INICIO.\n");
    printf("Presiona el boton para comenzar el monitoreo.\n");
    printf("Toda la informacion se mostrara por monitor serial.\n");
    printf("=============================================\n\n");

    /* =========================================================
     * 6. EJECUTAR MAQUINA DE ESTADOS
     * ========================================================= */
    ejecutar_maquina_estados(&contexto_sistema);

    /**
     * @brief Bucle de seguridad.
     *
     * En condiciones normales nunca se debería llegar aquí, ya que
     * la máquina de estados debería ejecutarse de manera continua.
     * Sin embargo, se deja este bucle infinito como protección para
     * evitar que la tarea principal termine inesperadamente.
     */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}