// src/setpoint.cpp

#include "setpoint.h"
#include "config.h"

void setpoint_generator_task(void *parameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // Executa a cada 100ms

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        unsigned long currentTime = millis();
        double new_sp = g_systemState.sp;

        // mudança do setpoint baseada no tempo
        if (currentTime >= (TIME_TO_DISCHARGE_MS + 5000) && currentTime < (TIME_TO_DISCHARGE_MS + 10000)) {
            new_sp = 0.8 * ADC_RESOLUTION; // ateh 80 porcento
        } else if (currentTime >= (TIME_TO_DISCHARGE_MS + 10000)) {
            new_sp = 0.2 * ADC_RESOLUTION; // comeca 20 porcento
        }

        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
            g_systemState.sp = new_sp;
            xSemaphoreGive(xStateMutex);
        }
    }
}