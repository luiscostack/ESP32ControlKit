// src/setpoint.cpp

#include "setpoint.h"
#include "config.h"

/**
 * @brief Tarefa que gera o perfil de setpoint (referência) ao longo do tempo.
 */
void setpoint_generator_task(void *parameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // Executa a cada 100ms

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        unsigned long currentTime = millis();
        double new_sp = g_systemState.sp;

        // Lógica para mudança do setpoint baseada no tempo
        if (currentTime >= (TIME_TO_DISCHARGE_MS + 5000) && currentTime < (TIME_TO_DISCHARGE_MS + 10000)) {
            new_sp = 0.8 * ADC_RESOLUTION; // 80% do setpoint máximo
        } else if (currentTime >= (TIME_TO_DISCHARGE_MS + 10000)) {
            new_sp = 0.2 * ADC_RESOLUTION; // 20% do setpoint máximo
        }

        // Bloqueia o mutex para atualizar o valor de setpoint com segurança
        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
            g_systemState.sp = new_sp;
            xSemaphoreGive(xStateMutex);
        }
    }
}