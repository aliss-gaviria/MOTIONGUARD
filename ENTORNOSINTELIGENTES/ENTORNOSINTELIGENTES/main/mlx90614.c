/**
 * @file mlx90614.c
 * @brief Implementación del módulo de adquisición de temperatura con el sensor MLX90614.
 *
 * Este archivo contiene la lógica necesaria para trabajar con el sensor
 * infrarrojo MLX90614, incluyendo:
 * - inicialización del bus I2C asociado al sensor
 * - lectura de temperatura desde registros internos
 * - adquisición de múltiples muestras
 * - cálculo de promedios de temperatura de objeto y ambiente
 *
 * Su propósito es encapsular toda la interacción con el MLX90614 para que
 * el resto del sistema pueda obtener mediciones térmicas de forma simple.
 */

#include "mlx90614.h"
#include "app_config.h"
#include "app_types.h"

#include <stdio.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Etiqueta usada para mensajes de log del módulo MLX90614.
 */
static const char *TAG_MLX90614 = "MLX90614";

/* ============================================================
 * INICIALIZACIÓN DEL BUS I2C
 * ============================================================ */

/**
 * @brief Inicializa el bus I2C utilizado por el sensor MLX90614.
 *
 * @return ESP_OK si la inicialización se realiza correctamente.
 * @return Código de error ESP-IDF si ocurre una falla.
 *
 * Lógica:
 * - configura el puerto I2C en modo maestro
 * - asigna los pines SDA y SCL definidos para este sensor
 * - habilita resistencias pull-up internas
 * - instala el driver I2C correspondiente
 *
 * Esta función prepara el canal de comunicación física necesario para
 * interactuar con el MLX90614.
 */
esp_err_t i2c_inicializar_mlx90614(void)
{
    i2c_config_t config_i2c_mlx = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_GPIO_SDA_MLX90614,
        .scl_io_num = I2C_GPIO_SCL_MLX90614,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FRECUENCIA_MLX90614_HZ,
        .clk_flags = 0
    };

    esp_err_t resultado = i2c_param_config(I2C_PUERTO_MLX90614, &config_i2c_mlx);
    if (resultado != ESP_OK) {
        ESP_LOGE(TAG_MLX90614, "Error config I2C MLX90614: %s", esp_err_to_name(resultado));
        return resultado;
    }

    resultado = i2c_driver_install(I2C_PUERTO_MLX90614, I2C_MODE_MASTER, 0, 0, 0);
    if (resultado != ESP_OK && resultado != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG_MLX90614, "Error instalando driver I2C MLX90614: %s", esp_err_to_name(resultado));
        return resultado;
    }

    ESP_LOGI(TAG_MLX90614, "I2C MLX90614 inicializado");
    return ESP_OK;
}

/* ============================================================
 * LECTURA DE TEMPERATURA
 * ============================================================ */

/**
 * @brief Lee una temperatura desde un registro del sensor MLX90614.
 *
 * @param registro Registro interno del sensor a consultar.
 * @param temperatura_celsius Puntero donde se almacenará la temperatura leída en grados Celsius.
 * @return ESP_OK si la lectura fue exitosa.
 * @return ESP_ERR_INVALID_ARG si el puntero de salida es nulo.
 * @return ESP_ERR_NO_MEM si no se puede crear el comando I2C.
 * @return Código de error ESP-IDF si la comunicación falla.
 *
 * Lógica:
 * - valida el puntero de salida
 * - crea una secuencia de comandos I2C
 * - escribe el registro a consultar
 * - reinicia la comunicación en modo lectura
 * - lee 3 bytes desde el sensor
 * - combina los dos primeros bytes en un valor de 16 bits
 * - verifica si el bit de error está activo
 * - convierte el valor crudo a Kelvin
 * - finalmente convierte a grados Celsius
 *
 * Esta función implementa la lectura básica del MLX90614 sobre I2C
 * y sirve tanto para temperatura ambiente como para temperatura de objeto.
 */
esp_err_t mlx90614_leer_temperatura(uint8_t registro, float *temperatura_celsius)
{
    if (temperatura_celsius == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer_rx[3];

    i2c_cmd_handle_t handle_cmd = i2c_cmd_link_create();
    if (handle_cmd == NULL) {
        return ESP_ERR_NO_MEM;
    }

    i2c_master_start(handle_cmd);
    i2c_master_write_byte(handle_cmd,
                          (MLX90614_DIRECCION_I2C << 1) | I2C_MASTER_WRITE,
                          true);
    i2c_master_write_byte(handle_cmd, registro, true);

    i2c_master_start(handle_cmd);
    i2c_master_write_byte(handle_cmd,
                          (MLX90614_DIRECCION_I2C << 1) | I2C_MASTER_READ,
                          true);
    i2c_master_read(handle_cmd, buffer_rx, 3, I2C_MASTER_LAST_NACK);
    i2c_master_stop(handle_cmd);

    esp_err_t resultado = i2c_master_cmd_begin(
        I2C_PUERTO_MLX90614,
        handle_cmd,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );

    i2c_cmd_link_delete(handle_cmd);

    if (resultado != ESP_OK) {
        ESP_LOGW(TAG_MLX90614, "Error leyendo MLX90614 reg 0x%02X: %s",
                 registro, esp_err_to_name(resultado));
        return resultado;
    }

    uint16_t valor_raw = ((uint16_t)buffer_rx[1] << 8) | buffer_rx[0];

    if (valor_raw & 0x8000) {
        ESP_LOGW(TAG_MLX90614, "Bit de error activo");
        return ESP_FAIL;
    }

    float temp_kelvin = (float)valor_raw * 0.02f;
    *temperatura_celsius = temp_kelvin - 273.15f;

    return ESP_OK;
}

/* ============================================================
 * TAREA DE MONITOREO DE TEMPERATURA
 * ============================================================ */

/**
 * @brief Ejecuta una ventana de monitoreo de temperatura con el MLX90614.
 *
 * @param modo_alerta Indica si debe usarse la cantidad de muestras de alerta
 * o la cantidad de muestras normal.
 * @param resultado Puntero a la estructura donde se almacenará el resultado final.
 *
 * Lógica:
 * - determina cuántas muestras tomar según el modo de operación
 * - inicializa acumuladores para temperatura de objeto y ambiente
 * - en cada iteración intenta leer ambos registros del sensor
 * - si ambas lecturas son exitosas, las acumula
 * - espera un tiempo entre muestras para espaciar la adquisición
 * - al final calcula el promedio usando solo las lecturas válidas
 * - actualiza la estructura de salida con temperaturas promedio,
 *   cantidad de muestras y estado de alerta
 *
 * Esta función permite obtener una medición térmica más estable que una
 * lectura única, ya que trabaja con una ventana de múltiples muestras.
 */
void tarea_monitorear_temperatura(bool modo_alerta, ResultadoTemperatura_t *resultado)
{
    uint32_t numero_muestras_temp = modo_alerta ? MUESTRAS_TEMP_ALERTA : MUESTRAS_TEMP_NORMAL;

    double suma_temp_objeto = 0.0;
    double suma_temp_ambiente = 0.0;
    uint32_t lecturas_exitosas = 0;

    printf("\n[TEMP] Iniciando monitoreo de temperatura...\n");

    for (uint32_t i = 0; i < numero_muestras_temp; i++) {
        float temp_objeto_actual = 0.0f;
        float temp_ambiente_actual = 0.0f;

        esp_err_t res_obj = mlx90614_leer_temperatura(MLX90614_REG_TOBJ, &temp_objeto_actual);
        esp_err_t res_amb = mlx90614_leer_temperatura(MLX90614_REG_TAMB, &temp_ambiente_actual);

        if (res_obj == ESP_OK && res_amb == ESP_OK) {
            suma_temp_objeto += temp_objeto_actual;
            suma_temp_ambiente += temp_ambiente_actual;
            lecturas_exitosas++;

            printf("[TEMP] Muestra %lu: Obj=%.2f C | Amb=%.2f C\n",
                   (unsigned long)(i + 1),
                   temp_objeto_actual,
                   temp_ambiente_actual);
        } else {
            printf("[TEMP] Muestra %lu: Error I2C\n", (unsigned long)(i + 1));
        }

        vTaskDelay(pdMS_TO_TICKS(DELAY_MUESTRAS_TEMP_MS));
    }

    if (lecturas_exitosas > 0) {
        resultado->temp_objeto_celsius = (float)(suma_temp_objeto / lecturas_exitosas);
        resultado->temp_ambiente_celsius = (float)(suma_temp_ambiente / lecturas_exitosas);
    } else {
        resultado->temp_objeto_celsius = 0.0f;
        resultado->temp_ambiente_celsius = 0.0f;
    }

    resultado->total_muestras = numero_muestras_temp;
    resultado->alerta_activa = false;

    printf("[TEMP] Promedio: Obj=%.2f C | Amb=%.2f C\n",
           resultado->temp_objeto_celsius,
           resultado->temp_ambiente_celsius);
}