/**
 * @file max30100.c
 * @brief Implementación del módulo de adquisición y procesamiento del sensor MAX30100.
 *
 * Este archivo contiene toda la lógica relacionada con el sensor MAX30100,
 * incluyendo:
 * - inicialización del bus I2C asociado al sensor
 * - configuración interna del MAX30100
 * - lectura de muestras crudas del FIFO
 * - procesamiento básico de la señal IR y RED
 * - detección de picos para cálculo de BPM
 * - estimación de calidad de señal
 * - cálculo aproximado de SpO2
 *
 * En términos generales, este módulo toma muestras crudas del sensor,
 * las acondiciona mediante filtrado y separación de componente DC/AC,
 * y a partir de ellas intenta obtener métricas útiles para el sistema
 * de monitoreo biomédico.
 */

#include "max30100.h"
#include "app_config.h"
#include "app_types.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Etiqueta usada para mensajes de log del módulo MAX30100.
 */
static const char *TAG_MAX30100 = "MAX30100";

/* =========================
 * REGISTROS DEL MAX30100
 * ========================= */
/**
 * @brief Definiciones de registros internos y constantes de configuración
 * del MAX30100.
 *
 * Este bloque agrupa las direcciones de registros usadas durante la
 * inicialización y lectura del sensor, así como valores de configuración
 * del modo SpO2, reset, tasa de muestreo, ancho de pulso y corriente
 * de LEDs.
 *
 * Estas constantes permiten trabajar con nombres legibles en lugar de
 * valores hexadecimales sueltos dentro del código.
 */
#define MAX30100_REG_INT_STATUS         0x00
#define MAX30100_REG_INT_ENABLE         0x01
#define MAX30100_REG_FIFO_WR_PTR        0x02
#define MAX30100_REG_OVF_COUNTER        0x03
#define MAX30100_REG_FIFO_RD_PTR        0x04
#define MAX30100_REG_FIFO_DATA          0x05
#define MAX30100_REG_MODE_CONFIG        0x06
#define MAX30100_REG_SPO2_CONFIG        0x07
#define MAX30100_REG_LED_CONFIG         0x09
#define MAX30100_REG_PART_ID            0xFF

#define MAX30100_MODE_SPO2              0x03
#define MAX30100_RESET_BIT              0x40
#define MAX30100_SPO2_HI_RES_EN         0x40
#define MAX30100_SR_100HZ               0x04
#define MAX30100_LED_PW_1600US          0x03
#define MAX30100_LED_CURR_27MA          0x0B

#define MAX30100_MAX_PICOS              20
#define MAX30100_MIN_MUESTRAS_VALIDAS   20

/* =========================
 * FUNCIONES PRIVADAS
 * ========================= */

/**
 * @brief Calcula la desviación estándar de un arreglo de datos.
 *
 * @param arreglo Arreglo de valores de entrada.
 * @param n Número de elementos del arreglo.
 * @param media Media previamente calculada del arreglo.
 * @return Desviación estándar de los datos.
 *
 * Lógica:
 * - recorre el arreglo
 * - calcula la diferencia de cada muestra respecto a la media
 * - acumula el cuadrado de esas diferencias
 * - al final retorna la raíz del promedio de los cuadrados
 *
 * Esta función se usa para medir qué tanta variación tiene la señal,
 * lo cual ayuda a saber si existe actividad pulsátil suficiente.
 */
static float calcular_desviacion_estandar(const float *arreglo, int n, float media)
{
    double suma_cuadrados = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = arreglo[i] - media;
        suma_cuadrados += diff * diff;
    }
    return (float)sqrt(suma_cuadrados / n);
}

/**
 * @brief Calcula el valor RMS de una señal.
 *
 * @param arreglo Arreglo de muestras.
 * @param n Número de elementos.
 * @return Valor RMS de la señal.
 *
 * Lógica:
 * - eleva cada muestra al cuadrado
 * - suma todos los cuadrados
 * - divide por el número de muestras
 * - obtiene la raíz cuadrada del resultado
 *
 * El valor RMS se usa aquí como medida de amplitud efectiva de la
 * componente AC de la señal.
 */
static float calcular_rms(const float *arreglo, int n)
{
    double suma = 0.0;
    for (int i = 0; i < n; i++) {
        suma += (double)arreglo[i] * (double)arreglo[i];
    }
    return (float)sqrt(suma / n);
}

/**
 * @brief Aplica un filtro pasa-bajos IIR simple a una señal.
 *
 * @param entrada Señal de entrada.
 * @param salida Arreglo donde se almacena la señal filtrada.
 * @param n Número de muestras.
 * @param alpha Coeficiente del filtro.
 *
 * Lógica:
 * - la primera muestra de salida se iguala a la primera entrada
 * - cada muestra siguiente se calcula usando una combinación entre
 *   la salida previa y la entrada actual
 *
 * Esto ayuda a suavizar la señal y reducir ruido de alta frecuencia
 * antes del procesamiento principal.
 */
static void filtro_pasabajos_iir(const float *entrada, float *salida, int n, float alpha)
{
    if (n <= 0) return;

    salida[0] = entrada[0];
    for (int i = 1; i < n; i++) {
        salida[i] = alpha * salida[i - 1] + (1.0f - alpha) * entrada[i];
    }
}

/**
 * @brief Elimina la componente DC de una señal.
 *
 * @param entrada Señal de entrada.
 * @param componente_ac Arreglo donde se guarda la componente AC resultante.
 * @param n Número de muestras.
 * @param alpha_dc Coeficiente del filtro de estimación DC.
 *
 * Lógica:
 * - estima lentamente el nivel DC de la señal
 * - resta esa estimación a cada muestra
 * - el resultado corresponde a la variación AC alrededor del nivel base
 *
 * Esta operación es importante en señales fotopletismográficas porque
 * el pulso se encuentra precisamente en la variación AC, mientras que
 * el nivel base depende del contacto, tejido y luz absorbida.
 */
static void eliminar_componente_dc(const float *entrada, float *componente_ac, int n, float alpha_dc)
{
    if (n <= 0) return;

    float estimacion_dc = entrada[0];
    for (int i = 0; i < n; i++) {
        estimacion_dc = alpha_dc * estimacion_dc + (1.0f - alpha_dc) * entrada[i];
        componente_ac[i] = entrada[i] - estimacion_dc;
    }
}

/**
 * @brief Detecta picos en la señal AC y calcula BPM.
 *
 * @param senal_ac Señal procesada sin componente DC.
 * @param n Número de muestras de la señal.
 * @param fs Frecuencia de muestreo en Hz.
 * @return BPM calculado, o -1 si no se puede determinar.
 *
 * Lógica:
 * - valida parámetros de entrada
 * - calcula media y desviación estándar
 * - define un umbral adaptativo basado en la variabilidad
 * - busca máximos locales que superen el umbral
 * - aplica un período refractario para evitar contar dos veces el mismo pulso
 * - calcula los intervalos entre picos detectados
 * - estima BPM a partir del intervalo promedio
 *
 * Si la señal es muy plana, no hay suficientes picos, o el BPM sale
 * fuera de rango fisiológico, la función retorna -1.
 */
static int detectar_picos_y_calcular_bpm(const float *senal_ac, int n, float fs)
{
    if (senal_ac == NULL || n < 10 || fs <= 0.0f) {
        return -1;
    }

    float media = 0.0f;
    for (int i = 0; i < n; i++) {
        media += senal_ac[i];
    }
    media /= (float)n;

    float sd = calcular_desviacion_estandar(senal_ac, n, media);

    if (sd < 0.2f) {
        return -1;
    }

    float umbral_adaptativo = media + 0.15f * sd;

    int periodo_refractario_muestras = (int)(0.25f * fs);

    int indices_picos[MAX30100_MAX_PICOS];
    int total_picos_detectados = 0;
    int indice_ultimo_pico = -1000;

    for (int i = 2; i < n - 2; i++) {
        bool es_maximo_local =
            senal_ac[i] > umbral_adaptativo &&
            senal_ac[i] > senal_ac[i - 1] &&
            senal_ac[i] > senal_ac[i - 2] &&
            senal_ac[i] >= senal_ac[i + 1] &&
            senal_ac[i] >= senal_ac[i + 2];

        if (es_maximo_local && (i - indice_ultimo_pico) >= periodo_refractario_muestras) {
            if (total_picos_detectados < MAX30100_MAX_PICOS) {
                indices_picos[total_picos_detectados++] = i;
            }
            indice_ultimo_pico = i;
        }
    }

    ESP_LOGD(TAG_MAX30100, "SD=%.3f | Umbral=%.3f | Picos=%d",
             sd, umbral_adaptativo, total_picos_detectados);

    if (total_picos_detectados < 2) {
        return -1;
    }

    float suma_intervalos = 0.0f;
    int intervalos_validos = 0;

    for (int i = 1; i < total_picos_detectados; i++) {
        int delta_muestras = indices_picos[i] - indices_picos[i - 1];
        float delta_segundos = (float)delta_muestras / fs;

        if (delta_segundos > 0.0f) {
            suma_intervalos += delta_segundos;
            intervalos_validos++;
        }
    }

    if (intervalos_validos == 0) {
        return -1;
    }

    float intervalo_promedio_seg = suma_intervalos / (float)intervalos_validos;
    float bpm = 60.0f / intervalo_promedio_seg;

    if (bpm < 40.0f || bpm > 180.0f) {
        return -1;
    }

    return (int)(bpm + 0.5f);
}

/**
 * @brief Estima la calidad de la señal proveniente del MAX30100.
 *
 * @param ir_promedio Promedio del canal IR.
 * @param ir_ac_rms RMS de la componente AC del canal IR.
 * @param red_promedio Promedio del canal rojo.
 * @param red_ac_rms RMS de la componente AC del canal rojo.
 * @param ir_pico_a_pico Amplitud pico a pico del canal IR.
 * @return Puntaje de calidad entre 0 y 100.
 *
 * Lógica:
 * - asigna puntajes parciales según umbrales empíricos
 * - evalúa presencia de contacto y amplitud útil de la señal
 * - limita el resultado final al rango 0 a 100
 *
 * Esta medida se usa como criterio previo para decidir si vale la pena
 * aceptar la medición o descartarla por señal débil.
 */
static int estimar_calidad_senal_max30100(float ir_promedio,
                                          float ir_ac_rms,
                                          float red_promedio,
                                          float red_ac_rms,
                                          float ir_pico_a_pico)
{
    int calidad = 0;

    if (ir_promedio > 12000.0f)    calidad += 25;
    if (red_promedio > 12000.0f)   calidad += 15;
    if (ir_ac_rms > 80.0f)         calidad += 20;
    if (red_ac_rms > 50.0f)        calidad += 10;
    if (ir_pico_a_pico > 300.0f)   calidad += 30;

    if (calidad > 100) calidad = 100;
    if (calidad < 0)   calidad = 0;

    return calidad;
}

/**
 * @brief Calcula una estimación aproximada de SpO2.
 *
 * @param ir_promedio Promedio DC del canal IR.
 * @param ir_ac_rms Componente AC efectiva del canal IR.
 * @param red_promedio Promedio DC del canal rojo.
 * @param red_ac_rms Componente AC efectiva del canal rojo.
 * @return SpO2 estimada en porcentaje, o -1.0 si no es válida.
 *
 * Lógica:
 * - verifica que existan niveles mínimos útiles en DC y AC
 * - calcula el ratio de razones entre canal rojo e infrarrojo
 * - aplica una fórmula empírica simplificada
 * - limita el resultado a un rango fisiológico razonable
 *
 * Esta estimación es aproximada y depende mucho de la calidad de señal,
 * del contacto y de la calibración real del sensor.
 */
static float calcular_spo2(float ir_promedio, float ir_ac_rms,
                            float red_promedio, float red_ac_rms)
{
    if (ir_promedio <= 1.0f || red_promedio <= 1.0f ||
        ir_ac_rms <= 5.0f   || red_ac_rms <= 5.0f) {
        return -1.0f;
    }

    float ratio_r = (red_ac_rms / red_promedio) / (ir_ac_rms / ir_promedio);

    float spo2 = 104.0f - 17.0f * ratio_r;

    if (spo2 > 100.0f) spo2 = 100.0f;
    if (spo2 < 80.0f)  spo2 = 80.0f;

    return spo2;
}

/* =========================
 * I2C PRIVADO
 * ========================= */

/**
 * @brief Escribe un valor en un registro del MAX30100.
 *
 * @param registro Dirección del registro.
 * @param dato Dato a escribir.
 * @return Código de estado ESP-IDF de la operación.
 *
 * Lógica:
 * - construye un buffer de transmisión con registro y dato
 * - envía ambos bytes al dispositivo usando el puerto I2C configurado
 *
 * Esta función encapsula las escrituras simples al sensor.
 */
static esp_err_t max30100_escribir_registro(uint8_t registro, uint8_t dato)
{
    uint8_t buffer_tx[2] = { registro, dato };

    return i2c_master_write_to_device(
        I2C_PUERTO_MAX30100,
        MAX30100_DIRECCION_I2C,
        buffer_tx,
        sizeof(buffer_tx),
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );
}

/**
 * @brief Lee uno o varios bytes desde un registro del MAX30100.
 *
 * @param registro Dirección del registro inicial.
 * @param datos Buffer donde se almacenarán los datos leídos.
 * @param longitud Número de bytes a leer.
 * @return Código de estado ESP-IDF de la operación.
 *
 * Lógica:
 * - primero transmite la dirección del registro
 * - luego realiza una lectura sobre el mismo dispositivo
 *
 * Esta función es la base de todas las lecturas del sensor.
 */
static esp_err_t max30100_leer_registro(uint8_t registro, uint8_t *datos, size_t longitud)
{
    return i2c_master_write_read_device(
        I2C_PUERTO_MAX30100,
        MAX30100_DIRECCION_I2C,
        &registro,
        1,
        datos,
        longitud,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );
}

/* =========================
 * API PÚBLICA
 * ========================= */

/**
 * @brief Inicializa el bus I2C utilizado por el MAX30100.
 *
 * @return ESP_OK si la inicialización fue exitosa, o un código de error
 * en caso contrario.
 *
 * Lógica:
 * - configura el puerto I2C en modo maestro
 * - asigna los pines SDA y SCL definidos en la configuración
 * - habilita resistencias pull-up internas
 * - instala el driver I2C del puerto correspondiente
 *
 * Esta función prepara el canal físico de comunicación antes de intentar
 * acceder al sensor.
 */
esp_err_t i2c_inicializar_max30100(void)
{
    i2c_config_t config_i2c_max30100 = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_GPIO_SDA_MAX30100,
        .scl_io_num = I2C_GPIO_SCL_MAX30100,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FRECUENCIA_MAX30100_HZ,
        .clk_flags = 0
    };

    esp_err_t resultado = i2c_param_config(I2C_PUERTO_MAX30100, &config_i2c_max30100);
    if (resultado != ESP_OK) {
        ESP_LOGE(TAG_MAX30100, "Error config I2C MAX30100: %s", esp_err_to_name(resultado));
        return resultado;
    }

    resultado = i2c_driver_install(I2C_PUERTO_MAX30100, I2C_MODE_MASTER, 0, 0, 0);
    if (resultado != ESP_OK && resultado != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG_MAX30100, "Error instalando driver I2C MAX30100: %s", esp_err_to_name(resultado));
        return resultado;
    }

    ESP_LOGI(TAG_MAX30100, "I2C MAX30100 inicializado");
    return ESP_OK;
}

/**
 * @brief Inicializa y configura internamente el sensor MAX30100.
 *
 * @return ESP_OK si la configuración se realiza correctamente, o un
 * código de error en caso de falla.
 *
 * Lógica:
 * - verifica comunicación leyendo el PART_ID
 * - ejecuta un reset por software
 * - limpia punteros y contadores del FIFO
 * - configura el modo SpO2
 * - define resolución, tasa de muestreo y corriente de LEDs
 * - deshabilita interrupciones porque el diseño usa lectura por sondeo
 *
 * Esta función deja el sensor listo para comenzar a entregar muestras.
 */
esp_err_t max30100_inicializar(void)
{
    uint8_t part_id = 0;
    esp_err_t resultado = max30100_leer_registro(MAX30100_REG_PART_ID, &part_id, 1);
    if (resultado != ESP_OK) {
        ESP_LOGE(TAG_MAX30100, "MAX30100 no responde");
        return resultado;
    }

    ESP_LOGI(TAG_MAX30100, "PART_ID = 0x%02X", part_id);

    resultado = max30100_escribir_registro(MAX30100_REG_MODE_CONFIG, MAX30100_RESET_BIT);
    if (resultado != ESP_OK) return resultado;
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_ERROR_CHECK(max30100_escribir_registro(MAX30100_REG_FIFO_WR_PTR, 0x00));
    ESP_ERROR_CHECK(max30100_escribir_registro(MAX30100_REG_OVF_COUNTER, 0x00));
    ESP_ERROR_CHECK(max30100_escribir_registro(MAX30100_REG_FIFO_RD_PTR, 0x00));

    ESP_ERROR_CHECK(max30100_escribir_registro(MAX30100_REG_MODE_CONFIG, MAX30100_MODE_SPO2));

    uint8_t config_spo2 = MAX30100_SPO2_HI_RES_EN |
                          (MAX30100_SR_100HZ << 2) |
                          MAX30100_LED_PW_1600US;
    ESP_ERROR_CHECK(max30100_escribir_registro(MAX30100_REG_SPO2_CONFIG, config_spo2));

    uint8_t config_leds = (MAX30100_LED_CURR_27MA << 4) | MAX30100_LED_CURR_27MA;
    ESP_ERROR_CHECK(max30100_escribir_registro(MAX30100_REG_LED_CONFIG, config_leds));

    ESP_ERROR_CHECK(max30100_escribir_registro(MAX30100_REG_INT_ENABLE, 0x00));

    ESP_LOGI(TAG_MAX30100, "MAX30100 configurado");
    return ESP_OK;
}

/**
 * @brief Lee una muestra cruda del FIFO del MAX30100.
 *
 * @param muestra Puntero a la estructura donde se almacenará la muestra.
 * @return ESP_OK si la lectura fue exitosa, o un código de error si falla.
 *
 * Lógica:
 * - valida que el puntero de salida no sea nulo
 * - lee 4 bytes consecutivos desde el FIFO
 * - reconstruye dos valores de 16 bits:
 *   uno para IR y otro para RED
 *
 * Esta función representa la lectura elemental del sensor.
 */
esp_err_t max30100_leer_muestra(MuestraMax30100_t *muestra)
{
    if (muestra == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buffer_rx[4];
    esp_err_t resultado = max30100_leer_registro(MAX30100_REG_FIFO_DATA, buffer_rx, 4);
    if (resultado != ESP_OK) {
        return resultado;
    }

    muestra->ir  = ((uint16_t)buffer_rx[0] << 8) | buffer_rx[1];
    muestra->red = ((uint16_t)buffer_rx[2] << 8) | buffer_rx[3];

    return ESP_OK;
}

/**
 * @brief Ejecuta una ventana de monitoreo de ritmo cardíaco usando el MAX30100.
 *
 * @param modo_alerta Indica si la adquisición debe hacerse con una ventana
 * ampliada de muestras.
 * @param resultado Puntero a la estructura donde se almacenará el resultado final.
 *
 * Lógica general:
 * - valida el puntero de salida
 * - define cuántas muestras tomar según el modo
 * - reserva memoria dinámica para buffers temporales
 * - adquiere las muestras crudas del sensor
 * - valida si hubo suficientes lecturas exitosas
 * - calcula promedios DC de canales IR y RED
 * - verifica si existe contacto con el sensor
 * - filtra la señal y elimina componente DC
 * - calcula RMS y amplitud pico a pico
 * - estima calidad de señal
 * - calcula BPM y SpO2 si la señal es utilizable
 * - llena la estructura de salida con el resultado final
 * - libera toda la memoria reservada
 *
 * En resumen, esta es la función principal del módulo, ya que transforma
 * datos crudos del sensor en resultados interpretables por el sistema.
 */
void tarea_monitorear_ritmo(bool modo_alerta, ResultadoRitmo_t *resultado)
{
    if (resultado == NULL) {
        ESP_LOGE(TAG_MAX30100, "Puntero resultado es NULL");
        return;
    }

    int numero_muestras = modo_alerta ? MUESTRAS_MAX30100_ALERTA : MUESTRAS_MAX30100_NORMAL;

    /* Buffers en heap para evitar estado compartido entre llamadas */
    MuestraMax30100_t *buffer_muestras_raw = malloc(numero_muestras * sizeof(MuestraMax30100_t));
    float *senal_ir_raw       = malloc(numero_muestras * sizeof(float));
    float *senal_red_raw      = malloc(numero_muestras * sizeof(float));
    float *senal_ir_filtrada  = malloc(numero_muestras * sizeof(float));
    float *senal_red_filtrada = malloc(numero_muestras * sizeof(float));
    float *senal_ir_ac        = malloc(numero_muestras * sizeof(float));
    float *senal_red_ac       = malloc(numero_muestras * sizeof(float));

    if (!buffer_muestras_raw || !senal_ir_raw || !senal_red_raw ||
        !senal_ir_filtrada   || !senal_red_filtrada ||
        !senal_ir_ac         || !senal_red_ac) {
        ESP_LOGE(TAG_MAX30100, "Sin memoria para buffers");
        resultado->bpm_calculado      = -1;
        resultado->spo2_estimada      = -1.0f;
        resultado->contacto_detectado = false;
        resultado->resultado_valido   = false;
        resultado->calidad_senal      = 0;
        resultado->ir_promedio        = 0.0f;
        strcpy(resultado->estado_texto, "Sin memoria");
        goto cleanup;
    }

    ESP_LOGI(TAG_MAX30100, "Iniciando monitoreo (%d muestras)...", numero_muestras);

    int muestras_leidas = 0;
    for (int i = 0; i < numero_muestras; i++) {
        if (max30100_leer_muestra(&buffer_muestras_raw[i]) == ESP_OK) {
            muestras_leidas++;
        } else {
            buffer_muestras_raw[i].ir  = 0;
            buffer_muestras_raw[i].red = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(DELAY_MUESTRAS_MAX30100_MS));
    }

    /* Limpiar FIFO para dejar el sensor en un estado consistente tras la captura */
    max30100_escribir_registro(MAX30100_REG_FIFO_WR_PTR, 0x00);
    max30100_escribir_registro(MAX30100_REG_OVF_COUNTER, 0x00);
    max30100_escribir_registro(MAX30100_REG_FIFO_RD_PTR, 0x00);
    vTaskDelay(pdMS_TO_TICKS(50));

    if (muestras_leidas < MAX30100_MIN_MUESTRAS_VALIDAS) {
        resultado->bpm_calculado      = -1;
        resultado->spo2_estimada      = -1.0f;
        resultado->contacto_detectado = false;
        resultado->resultado_valido   = false;
        resultado->calidad_senal      = 0;
        resultado->ir_promedio        = 0.0f;
        strcpy(resultado->estado_texto, "Error de lectura I2C");
        goto cleanup;
    }

    double suma_ir = 0.0, suma_red = 0.0;
    for (int i = 0; i < numero_muestras; i++) {
        senal_ir_raw[i]  = (float)buffer_muestras_raw[i].ir;
        senal_red_raw[i] = (float)buffer_muestras_raw[i].red;
        suma_ir  += buffer_muestras_raw[i].ir;
        suma_red += buffer_muestras_raw[i].red;
    }

    resultado->ir_promedio = (float)(suma_ir / numero_muestras);
    float red_promedio     = (float)(suma_red / numero_muestras);

    resultado->contacto_detectado = (resultado->ir_promedio > 10000.0f);

    if (!resultado->contacto_detectado) {
        resultado->bpm_calculado    = -1;
        resultado->spo2_estimada    = -1.0f;
        resultado->resultado_valido = false;
        resultado->calidad_senal    = 0;
        strcpy(resultado->estado_texto, "Sin contacto");
        goto cleanup;
    }

    filtro_pasabajos_iir(senal_ir_raw,  senal_ir_filtrada,  numero_muestras, 0.70f);
    filtro_pasabajos_iir(senal_red_raw, senal_red_filtrada, numero_muestras, 0.70f);

    eliminar_componente_dc(senal_ir_filtrada,  senal_ir_ac,  numero_muestras, 0.90f);
    eliminar_componente_dc(senal_red_filtrada, senal_red_ac, numero_muestras, 0.90f);

    float ir_ac_rms  = calcular_rms(senal_ir_ac,  numero_muestras);
    float red_ac_rms = calcular_rms(senal_red_ac, numero_muestras);

    float ir_max = senal_ir_ac[0], ir_min = senal_ir_ac[0];
    for (int i = 1; i < numero_muestras; i++) {
        if (senal_ir_ac[i] > ir_max) ir_max = senal_ir_ac[i];
        if (senal_ir_ac[i] < ir_min) ir_min = senal_ir_ac[i];
    }
    float ir_pico_a_pico = ir_max - ir_min;

    resultado->calidad_senal = estimar_calidad_senal_max30100(
        resultado->ir_promedio, ir_ac_rms,
        red_promedio, red_ac_rms,
        ir_pico_a_pico
    );

    if (resultado->calidad_senal < 35) {
        resultado->bpm_calculado    = -1;
        resultado->spo2_estimada    = -1.0f;
        resultado->resultado_valido = false;
        strcpy(resultado->estado_texto, "Señal débil");
        goto cleanup;
    }

    resultado->bpm_calculado = detectar_picos_y_calcular_bpm(
        senal_ir_ac, numero_muestras, (float)MAX30100_SAMPLE_RATE_HZ
    );

    resultado->spo2_estimada = calcular_spo2(
        resultado->ir_promedio, ir_ac_rms, red_promedio, red_ac_rms
    );

    if (resultado->bpm_calculado < 0) {
        resultado->resultado_valido = false;
        strcpy(resultado->estado_texto, "Pulso inestable");
    } else if (resultado->spo2_estimada < 0.0f) {
        resultado->resultado_valido = false;
        strcpy(resultado->estado_texto, "SpO2 inestable");
    } else {
        resultado->resultado_valido = true;
        strcpy(resultado->estado_texto, "Lectura válida");
    }

    ESP_LOGI(TAG_MAX30100, "BPM: %d | SpO2: %.1f%% | Calidad: %d/100",
             resultado->bpm_calculado, resultado->spo2_estimada, resultado->calidad_senal);
    ESP_LOGD(TAG_MAX30100, "IR prom: %.2f | IR AC RMS: %.2f | RED AC RMS: %.2f | P-P: %.2f",
             resultado->ir_promedio, ir_ac_rms, red_ac_rms, ir_pico_a_pico);

cleanup:
    free(buffer_muestras_raw);
    free(senal_ir_raw);
    free(senal_red_raw);
    free(senal_ir_filtrada);
    free(senal_red_filtrada);
    free(senal_ir_ac);
    free(senal_red_ac);
}