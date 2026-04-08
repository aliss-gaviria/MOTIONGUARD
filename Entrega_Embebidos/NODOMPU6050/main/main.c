/**
 * @file main.c
 * @brief Nodo de adquisición de datos usando ESP32 y MPU6050.
 *
 * Este archivo implementa la lógica principal del nodo sensor. El sistema
 * utiliza un microcontrolador ESP32 conectado a un sensor MPU6050 para
 * adquirir datos de aceleración, velocidad angular y temperatura.
 *
 * Los datos se procesan y posteriormente se envían de forma inalámbrica
 * utilizando el protocolo ESP-NOW. Además, el sistema incluye un LED
 * indicador de estado y utiliza el modo Deep Sleep para optimizar el
 * consumo energético.
 *
 * Arquitectura del sistema:
 * - Sensor MPU6050 (acelerómetro + giroscopio + temperatura)
 * - Comunicación inalámbrica ESP-NOW
 * - FreeRTOS para manejo de tareas
 * - LED indicador de actividad
 *
 * Flujo de funcionamiento:
 * 1. Inicialización de periféricos
 * 2. Calibración del sensor MPU6050
 * 3. Creación de tareas FreeRTOS
 * 4. Adquisición de muestras del sensor
 * 5. Envío de datos vía ESP-NOW
 * 6. Entrada en modo Deep Sleep
 */

#include "config.h"
#include "mpu6050.h"
#include "espnow_comm.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_sleep.h"

/**
 * @brief Tag utilizado para los mensajes de log del sistema.
 */
static const char *TAG = "MAIN";

/**
 * @brief Variable compartida que almacena los datos más recientes del sensor.
 *
 * Esta estructura contiene las últimas mediciones obtenidas del sensor
 * MPU6050 (aceleración, giroscopio y temperatura). Es accedida por
 * múltiples tareas, por lo que su acceso está protegido mediante un mutex.
 */
static mpu_values_t s_shared_data;

/**
 * @brief Mutex utilizado para proteger el acceso a los datos compartidos.
 */
static SemaphoreHandle_t s_data_mutex;

/**
 * @brief Inicializa el LED de estado.
 *
 * Configura el pin definido en `LED_PIN` como salida digital para
 * permitir indicar el estado del nodo mediante encendido o apagado.
 */
static void led_init(void)
{
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

/**
 * @brief Tarea FreeRTOS encargada de la adquisición de datos del sensor.
 *
 * Esta tarea realiza la lectura periódica del sensor MPU6050 durante
 * un número determinado de muestras definido por `SAMPLE_COUNT`.
 *
 * Cada lectura se almacena en la variable compartida protegida por
 * un mutex para permitir que otra tarea pueda enviarla mediante
 * comunicación inalámbrica.
 *
 * @param pvParameters Parámetros de la tarea (no utilizados).
 */
static void task_sensor(void *pvParameters)
{
    ESP_LOGI(TAG, "[SENSOR] Iniciada");

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        mpu_values_t temp;

        if (mpu_get_values(&temp) == ESP_OK) {

            xSemaphoreTake(s_data_mutex, portMAX_DELAY);
            s_shared_data = temp;
            xSemaphoreGive(s_data_mutex);

            ESP_LOGI(TAG, "[SENSOR] Muestra %d/%d tomada", i + 1, SAMPLE_COUNT);
        } else {
            ESP_LOGE(TAG, "[SENSOR] Error en muestra %d", i + 1);
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }

    ESP_LOGI(TAG, "[SENSOR] Finalizada");
    vTaskDelete(NULL);
}
/**
 * @brief Tarea encargada del envío de datos y gestión del modo de bajo consumo.
 *
 * Esta tarea obtiene las muestras almacenadas por la tarea de adquisición
 * y las transmite mediante el protocolo ESP-NOW hacia el nodo receptor.
 *
 * Una vez que todas las muestras han sido enviadas, el sistema entra en
 * modo Deep Sleep durante un intervalo definido por `DEEP_SLEEP_SEC`
 * con el fin de reducir el consumo energético del dispositivo.
 *
 * @param pvParameters Parámetros de la tarea (no utilizados).
 */
static void task_send(void *pvParameters)
{
    ESP_LOGI(TAG, "[SEND] Iniciada");

    for (int i = 0; i < SAMPLE_COUNT; i++) {

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));

        mpu_values_t data;

        xSemaphoreTake(s_data_mutex, portMAX_DELAY);
        data = s_shared_data;
        xSemaphoreGive(s_data_mutex);

        ESP_LOGI(TAG, "[SEND] Enviando muestra %d/%d...", i + 1, SAMPLE_COUNT);

        send_data_espnow(&data);

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " Entrando en Deep Sleep %llus...", DEEP_SLEEP_SEC);
    ESP_LOGI(TAG, "==============================================");

    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(300));

    gpio_set_level(LED_PIN, 0);

    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_SEC * 1000000ULL);
    esp_deep_sleep_start();
}

/**
 * @brief Función principal del sistema.
 *
 * Esta función representa el punto de entrada del firmware en ESP-IDF.
 * En ella se realizan las siguientes operaciones:
 *
 * - Verificación de la causa de arranque del sistema
 * - Inicialización del LED indicador
 * - Inicialización del sensor MPU6050
 * - Calibración del sensor
 * - Inicialización de la comunicación ESP-NOW
 * - Creación del mutex de sincronización
 * - Creación de tareas FreeRTOS
 *
 * El sistema queda posteriormente gestionado por las tareas
 * `task_sensor` y `task_send`.
 */
void app_main(void)
{
    uint32_t causas = esp_sleep_get_wakeup_causes();

    if (causas & BIT(ESP_SLEEP_WAKEUP_TIMER))
        ESP_LOGI(TAG, "*** REINICIO desde DEEP SLEEP (timer) ***");
    else
        ESP_LOGI(TAG, "*** INICIO NORMAL ***");

    led_init();
    gpio_set_level(LED_PIN, 1);

    mpu6050_init();
    mpu6050_calibrate();
    espnow_init();

    s_data_mutex = xSemaphoreCreateMutex();

    if (s_data_mutex == NULL) {
        ESP_LOGE(TAG, "Error creando mutex");
        return;
    }

    ESP_LOGI(TAG, "Creando tareas...");

    xTaskCreate(task_sensor, "task_sensor", 4096, NULL, 5, NULL);
    xTaskCreate(task_send,   "task_send",   4096, NULL, 4, NULL);
}