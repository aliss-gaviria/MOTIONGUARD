/**
 * @file lcd_task.cpp
 * @brief Tarea encargada de la visualización en pantalla LCD.
 *
 * @details
 * Este módulo:
 * - Lee datos compartidos (sensor y batería)
 * - Dibuja interfaz gráfica
 * - Muestra valores en tiempo real
 * - Usa framebuffer en memoria dinámica
 */

#include "lcd.h"
#include "config.h"

/**
 * @brief Tarea de actualización de pantalla.
 *
 * @param pvParameters Parámetros FreeRTOS (no utilizados).
 *
 * @note
 * Ejecutada en núcleo independiente para evitar bloqueos.
 */
void TaskLCD(void *pvParameters)
{
    while (true)
    {
        float acc_local[3], gyro_local[3], bat_local;

        if (xSemaphoreTake(xDisplayMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            memcpy(acc_local, acc, sizeof(acc));
            memcpy(gyro_local, gyro, sizeof(gyro));
            bat_local = bateria;
            xSemaphoreGive(xDisplayMutex);
        }

        Paint_DrawRectangle(0, 0, 240, 240, 0XF410, DOT_PIXEL_2X2, DRAW_FILL_FULL);

        Paint_DrawString_EN(45, 50, "ACC_X =", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(45, 75, "ACC_Y =", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(45, 100, "ACC_Z =", &Font16, WHITE, BLACK);

        Paint_DrawString_EN(45, 125, "GYR_X =", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(45, 150, "GYR_Y =", &Font16, WHITE, BLACK);
        Paint_DrawString_EN(45, 175, "GYR_Z =", &Font16, WHITE, BLACK);

        Paint_DrawNum(140, 50, acc_local[0]/ACC_SCALE, &Font16, 4, BLACK, WHITE);
        Paint_DrawNum(140, 75, acc_local[1]/ACC_SCALE, &Font16, 4, BLACK, WHITE);
        Paint_DrawNum(140, 100, acc_local[2]/ACC_SCALE, &Font16, 4, BLACK, WHITE);

        Paint_DrawNum(140, 125, gyro_local[0]/GYRO_SCALE, &Font16, 4, BLACK, WHITE);
        Paint_DrawNum(140, 150, gyro_local[1]/GYRO_SCALE, &Font16, 4, BLACK, WHITE);
        Paint_DrawNum(140, 175, gyro_local[2]/GYRO_SCALE, &Font16, 4, BLACK, WHITE);

        LCD_1IN28_Display(BlackImage);

        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}