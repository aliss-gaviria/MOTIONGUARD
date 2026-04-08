/**
 * @file app_types.h
 * @brief Definición de estructuras de datos del sistema de monitoreo biomédico.
 *
 * Este archivo agrupa los tipos de datos principales utilizados en el proyecto
 * para almacenar:
 * - muestras crudas del sensor MAX30100
 * - resultados procesados de ritmo cardíaco
 * - resultados procesados de temperatura
 * - contexto general de la máquina de estados
 *
 * Su propósito es centralizar las estructuras compartidas entre módulos,
 * facilitando la organización del código y el intercambio de información
 * entre los archivos del sistema.
 */

#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "estados.h"

/* ============================================================
 * ESTRUCTURA DE MUESTRA DEL MAX30100
 * ============================================================ */
/**
 * @brief Estructura que almacena una muestra cruda del sensor MAX30100.
 *
 * Cada lectura del MAX30100 entrega dos canales principales:
 * - IR: canal infrarrojo, usado principalmente para detección de pulso
 * - RED: canal rojo, utilizado junto con IR para estimación de SpO2
 *
 * Esta estructura representa una sola muestra adquirida desde el FIFO
 * del sensor, antes de aplicar cualquier procesamiento.
 */
typedef struct {
    uint16_t ir;     /**< Valor crudo del canal infrarrojo */
    uint16_t red;    /**< Valor crudo del canal rojo */
} MuestraMax30100_t;

/* ============================================================
 * ESTRUCTURA DE RESULTADO DE RITMO CARDÍACO
 * ============================================================ */
/**
 * @brief Estructura que almacena el resultado procesado del monitoreo
 * de ritmo cardíaco.
 *
 * Esta estructura reúne toda la información relevante obtenida a partir
 * del procesamiento de las muestras del MAX30100, incluyendo:
 * - BPM calculado
 * - estimación de SpO2
 * - promedio del canal IR
 * - estado de contacto con el sensor
 * - validez del resultado
 * - calidad de la señal
 * - descripción textual del estado de la medición
 *
 * Su lógica es encapsular en una sola variable todo el resultado del
 * análisis de una ventana de monitoreo del sensor cardíaco.
 */
typedef struct {
    int bpm_calculado;              /**< Ritmo cardíaco calculado en latidos por minuto */
    float spo2_estimada;            /**< Saturación estimada de oxígeno */
    float ir_promedio;              /**< Promedio del canal IR, útil para verificar contacto */
    bool contacto_detectado;        /**< Indica si el dedo fue detectado correctamente */
    bool resultado_valido;          /**< Indica si la medición final es confiable */
    int calidad_senal;              /**< Calidad estimada de la señal adquirida */
    char estado_texto[64];          /**< Mensaje descriptivo del estado de la medición */
} ResultadoRitmo_t;

/* ============================================================
 * ESTRUCTURA DE RESULTADO DE TEMPERATURA
 * ============================================================ */
/**
 * @brief Estructura que almacena el resultado procesado del monitoreo
 * de temperatura.
 *
 * Contiene los datos calculados a partir de una ventana de lectura del
 * sensor MLX90614:
 * - temperatura del objeto
 * - temperatura ambiente
 * - número total de muestras consideradas
 * - estado de alerta asociado a la lectura
 *
 * Esta estructura permite mantener agrupado el resultado térmico
 * para su posterior evaluación dentro de la lógica del sistema.
 */
typedef struct {
    float temp_objeto_celsius;      /**< Temperatura promedio del objeto en grados Celsius */
    float temp_ambiente_celsius;    /**< Temperatura promedio del ambiente en grados Celsius */
    uint32_t total_muestras;        /**< Número total de muestras tomadas en la ventana */
    bool alerta_activa;             /**< Indica si la lectura térmica activó una alerta */
} ResultadoTemperatura_t;

/* ============================================================
 * ESTRUCTURA DE CONTEXTO DEL SISTEMA
 * ============================================================ */
/**
 * @brief Estructura que almacena el contexto general de ejecución del sistema.
 *
 * Esta estructura guarda la información necesaria para que la máquina
 * de estados mantenga continuidad entre ciclos de ejecución:
 * - estado actual del sistema
 * - cantidad de alertas amarillas acumuladas
 * - cantidad de alertas rojas acumuladas
 * - última temperatura registrada para comparar cambios bruscos
 *
 * Su lógica es servir como memoria del sistema durante la ejecución
 * del programa, permitiendo tomar decisiones basadas en eventos previos.
 */
typedef struct {
    estado_sistema_t estado_actual;             /**< Estado actual de la máquina de estados */
    uint32_t contador_alertas_amarillas;        /**< Contador acumulado de alertas amarillas */
    uint32_t contador_alertas_rojas;            /**< Contador acumulado de alertas rojas */
    float temperatura_anterior_celsius;         /**< Última temperatura registrada para comparación */
} ContextoSistema_t;

#endif