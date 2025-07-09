// src/controller.cpp

#include "controller.h"
#include "config.h"

/**
 * @brief Inicializa o controlador com ganhos e limites.
 */
void controller_init(double kP, double kI, double kD) {
    g_systemState.u = 0;
    g_systemState.y = 0;
    g_systemState.iTerm = 0;
    g_systemState.lastY = 0;
    g_systemState.sp = 0.5 * ADC_RESOLUTION; // Ponto inicial (50%)

    controller_set_tunings(kP, kI, kD);
}

/**
 * @brief Configura os ganhos do PID, ajustando para o tempo de amostragem.
 */
void controller_set_tunings(double kP, double kI, double kD) {
    double sampleTimeInSec = (double)SAMPLE_TIME_MS / 1000.0;
    g_systemState.kp = kP;
    g_systemState.ki = kI * sampleTimeInSec;
    g_systemState.kd = kD / sampleTimeInSec;
}

/**
 * @brief Calcula o sinal de controle PID.
 * @note Esta função deve ser chamada após o Mutex ser obtido,
 * pois acessa e modifica a estrutura g_systemState.
 */
void controller_compute() {
    // Erro de rastreamento
    double e = g_systemState.sp - g_systemState.y;

    // Termo Integral com anti-windup
    g_systemState.iTerm += (g_systemState.ki * e);
    if (g_systemState.iTerm > DAC_RESOLUTION) g_systemState.iTerm = DAC_RESOLUTION;
    else if (g_systemState.iTerm < 0) g_systemState.iTerm = 0;

    // Termo Derivativo (sobre a medição para evitar 'derivative kick')
    double dY = g_systemState.y - g_systemState.lastY;

    // Calcula o sinal de controle PID
    g_systemState.u = (g_systemState.kp * e) + g_systemState.iTerm - (g_systemState.kd * dY);

    // Saturação do sinal de controle
    if (g_systemState.u > DAC_RESOLUTION) g_systemState.u = DAC_RESOLUTION;
    else if (g_systemState.u < 0) g_systemState.u = 0;

    // Atualiza a última leitura
    g_systemState.lastY = g_systemState.y;
}