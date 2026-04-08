/**
 * @file sistema.c
 * @brief Implementación de la lógica principal del sistema de monitoreo biomédico.
 *
 * Este módulo contiene la lógica de alto nivel del sistema, incluyendo:
 * - evaluación de alertas rojas por ritmo cardíaco
 * - evaluación de alertas amarillas por temperatura
 * - verificación de posible crisis según acumulación de alertas
 * - ejecución de la máquina de estados principal
 *
 * Su función es coordinar el comportamiento general del sistema a partir
 * de los resultados entregados por los módulos de sensores.
 */

#include "sistema.h"
#include "app_config.h"
#include "estados.h"
#include "app_types.h"

#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "max30100.h"
#include "mlx90614.h"
#include "control.h"

/* ============================================================
 * EVALUACIÓN DE ALERTA ROJA
 * ============================================================ */

/**
 * @brief Evalúa si una medición de ritmo cardíaco genera alerta roja.
 *
 * @param resultado Puntero a la estructura con el resultado del monitoreo de ritmo.
 * @param ctx Puntero al contexto global del sistema.
 * @return true si se activa una alerta roja.
 * @return false si no se activa alerta roja.
 *
 * Lógica:
 * - primero verifica que el resultado sea válido
 * - compara el BPM calculado contra los umbrales mínimo y máximo
 * - si el valor está fuera de rango, incrementa el contador de alertas rojas
 * - muestra la información correspondiente por consola
 *
 * Esta función solo considera una alerta real cuando la medición es confiable,
 * evitando falsos positivos por lecturas inválidas.
 */
bool evaluar_alerta_roja(const ResultadoRitmo_t *resultado, ContextoSistema_t *ctx)
{
    if (!resultado->resultado_valido) {
        printf("[ALERTA] Resultado de ritmo no válido\n");
        return false;
    }

    bool fuera_de_rango = (resultado->bpm_calculado < UMBRAL_BPM_MINIMO ||
                           resultado->bpm_calculado > UMBRAL_BPM_MAXIMO);

    if (fuera_de_rango) {
        ctx->contador_alertas_rojas++;

        printf("\n=====================================\n");
        printf("           ALERTA ROJA\n");
        printf("=====================================\n");
        printf("BPM: %d lpm\n", resultado->bpm_calculado);
        printf("SpO2: %.1f %%\n", resultado->spo2_estimada);
        printf("Alertas rojas: %lu\n", (unsigned long)ctx->contador_alertas_rojas);
        printf("=====================================\n\n");

        return true;
    }

    return false;
}

/* ============================================================
 * EVALUACIÓN DE ALERTA AMARILLA
 * ============================================================ */

/**
 * @brief Evalúa si una medición de temperatura genera alerta amarilla.
 *
 * @param resultado Puntero a la estructura con el resultado del monitoreo térmico.
 * @param ctx Puntero al contexto global del sistema.
 * @return true si se activa una alerta amarilla.
 * @return false si no se activa alerta amarilla.
 *
 * Lógica:
 * - compara la temperatura actual con la temperatura anterior
 * - determina si hubo un cambio abrupto significativo
 * - verifica si además la temperatura supera el umbral de alerta
 * - actualiza la temperatura anterior en el contexto
 * - si ambas condiciones se cumplen, incrementa el contador de alertas amarillas
 *
 * Es decir, no basta con que la temperatura esté alta: también debe existir
 * una variación relevante respecto a la lectura previa.
 */
bool evaluar_alerta_amarilla(const ResultadoTemperatura_t *resultado, ContextoSistema_t *ctx)
{
    bool cambio_abrupto = false;

    if (ctx->temperatura_anterior_celsius > 0.0f) {
        float delta = resultado->temp_objeto_celsius - ctx->temperatura_anterior_celsius;
        if (delta < 0) delta = -delta;

        if (delta >= CAMBIO_ABRUPTO_TEMPERATURA_C) {
            cambio_abrupto = true;
        }
    }

    bool temperatura_elevada = (resultado->temp_objeto_celsius > UMBRAL_TEMP_ALERTA_CELSIUS);

    ctx->temperatura_anterior_celsius = resultado->temp_objeto_celsius;

    if (temperatura_elevada && cambio_abrupto) {
        ctx->contador_alertas_amarillas++;

        printf("\n=====================================\n");
        printf("         ALERTA AMARILLA\n");
        printf("=====================================\n");
        printf("Temperatura: %.2f C\n", resultado->temp_objeto_celsius);
        printf("Alertas amarillas: %lu\n", (unsigned long)ctx->contador_alertas_amarillas);
        printf("=====================================\n\n");

        return true;
    }

    return false;
}

/* ============================================================
 * VERIFICACIÓN DE POSIBLE CRISIS
 * ============================================================ */

/**
 * @brief Verifica si el sistema ha alcanzado una condición de posible crisis.
 *
 * @param ctx Puntero al contexto global del sistema.
 * @return true si se detecta posible crisis.
 * @return false si no se cumple la condición.
 *
 * Lógica:
 * - compara la cantidad acumulada de alertas amarillas y rojas
 *   contra sus límites configurados
 * - si alguno de los contadores supera el máximo permitido,
 *   se informa una posible crisis por consola
 *
 * Esta función trabaja a nivel acumulado, no sobre una sola medición.
 */
bool verificar_posible_crisis(const ContextoSistema_t *ctx)
{
    bool crisis_temperatura = (ctx->contador_alertas_amarillas >= MAX_ALERTAS_AMARILLAS);
    bool crisis_ritmo = (ctx->contador_alertas_rojas >= MAX_ALERTAS_ROJAS);

    if (crisis_temperatura || crisis_ritmo) {
        printf("\n#####################################\n");
        printf("    POSIBLE CRISIS DE ANSIEDAD\n");
        printf("#####################################\n");
        printf("Alertas amarillas: %lu / %d\n",
               (unsigned long)ctx->contador_alertas_amarillas,
               MAX_ALERTAS_AMARILLAS);
        printf("Alertas rojas: %lu / %d\n",
               (unsigned long)ctx->contador_alertas_rojas,
               MAX_ALERTAS_ROJAS);
        printf("#####################################\n\n");
        return true;
    }

    return false;
}

/* ============================================================
 * MÁQUINA DE ESTADOS PRINCIPAL
 * ============================================================ */

/**
 * @brief Ejecuta la máquina de estados principal del sistema.
 *
 * @param ctx Puntero al contexto global del sistema.
 *
 * Lógica general:
 * - declara estructuras locales para almacenar resultados de ritmo y temperatura
 * - mantiene dos banderas para indicar si el siguiente ciclo debe ejecutarse
 *   en modo alerta
 * - entra en un bucle infinito que representa la operación continua del sistema
 * - verifica si el botón fue presionado para regresar al estado inicial
 * - según el estado actual:
 *   - en `ESTADO_INICIO`, espera una pulsación para comenzar el monitoreo
 *   - en `ESTADO_MONITOREO_RITMO`, mide ritmo cardíaco y evalúa alerta roja
 *   - en `ESTADO_MONITOREO_TEMPERATURA`, mide temperatura y evalúa alerta amarilla
 * - después de cada medición, verifica si existe posible crisis
 * - alterna entre ritmo y temperatura mientras el sistema esté activo
 *
 * En resumen, esta función es el núcleo de control del sistema, ya que define
 * cómo fluye la operación entre estados y cómo se toman decisiones a partir
 * de las mediciones.
 */
void ejecutar_maquina_estados(ContextoSistema_t *ctx)
{
    ResultadoRitmo_t resultado_ritmo = {0};
    ResultadoTemperatura_t resultado_temperatura = {0};

    bool modo_alerta_ritmo = false;
    bool modo_alerta_temperatura = false;

    while (1) {
        if (boton_esta_presionado()) {
            ctx->estado_actual = ESTADO_INICIO;
            led_apagar();
            printf("\n[SISTEMA] Botón presionado -> regreso a INICIO\n");
            vTaskDelay(pdMS_TO_TICKS(300));
        }

        switch (ctx->estado_actual) {
            case ESTADO_INICIO:
                led_apagar();
                printf("[ESTADO] INICIO\n");

                if (boton_esta_presionado()) {
                    ctx->estado_actual = ESTADO_MONITOREO_RITMO;
                    printf("[SISTEMA] Pasando a MONITOREO_RITMO\n");
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                break;

            case ESTADO_MONITOREO_RITMO:
                led_encender();
                printf("[ESTADO] MONITOREO_RITMO\n");

                tarea_monitorear_ritmo(modo_alerta_ritmo, &resultado_ritmo);
                modo_alerta_ritmo = evaluar_alerta_roja(&resultado_ritmo, ctx);
                verificar_posible_crisis(ctx);

                ctx->estado_actual = ESTADO_MONITOREO_TEMPERATURA;
                break;

            case ESTADO_MONITOREO_TEMPERATURA:
                led_encender();
                printf("[ESTADO] MONITOREO_TEMPERATURA\n");

                tarea_monitorear_temperatura(modo_alerta_temperatura, &resultado_temperatura);
                modo_alerta_temperatura = evaluar_alerta_amarilla(&resultado_temperatura, ctx);
                verificar_posible_crisis(ctx);

                ctx->estado_actual = ESTADO_MONITOREO_RITMO;
                break;

            default:
                ctx->estado_actual = ESTADO_INICIO;
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}