/**
 * @file control.h
 * @brief Declaración de funciones para el manejo del LED y del botón del sistema.
 *
 * Este módulo define la interfaz pública para controlar las entradas y salidas
 * digitales básicas del proyecto. En particular, permite:
 * - inicializar el LED indicador
 * - inicializar el botón de usuario
 * - encender y apagar el LED
 * - consultar si el botón ha sido presionado
 *
 * Su objetivo es separar la lógica de control GPIO del resto del sistema,
 * facilitando la organización y reutilización del código.
 */

#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>

/**
 * @brief Inicializa el pin del LED como salida digital.
 *
 * Esta función prepara el GPIO correspondiente al LED para que pueda
 * ser controlado por software durante la ejecución del sistema.
 */
void led_inicializar(void);

/**
 * @brief Inicializa el pin del botón como entrada digital.
 *
 * Configura el GPIO del botón para ser leído por el sistema,
 * normalmente usando resistencia pull-up interna.
 */
void boton_inicializar(void);

/**
 * @brief Enciende el LED del sistema.
 *
 * Coloca el pin del LED en el nivel lógico necesario para activar
 * la señal visual.
 */
void led_encender(void);

/**
 * @brief Apaga el LED del sistema.
 *
 * Coloca el pin del LED en el nivel lógico necesario para desactivar
 * la señal visual.
 */
void led_apagar(void);

/**
 * @brief Verifica si el botón se encuentra presionado.
 *
 * @return true si se detecta una pulsación válida.
 * @return false si el botón no está presionado.
 *
 * Esta función puede incluir validación básica para evitar lecturas
 * erróneas por rebote mecánico.
 */
bool boton_esta_presionado(void);

#endif