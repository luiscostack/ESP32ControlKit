// src/controller.cpp

#include "controller.h"
#include "config.h"


void controller_init(double kP, double kI, double kD) {
    g_systemState.u = 0;
    g_systemState.y = 0;
    g_systemState.iTerm = 0;
    g_systemState.lastY = 0;
    g_systemState.sp = 0.5 * ADC_RESOLUTION; // Ponto inicial (50%)

    controller_set_tunings(kP, kI, kD);
}

void controller_set_tunings(double kP, double kI, double kD) {
    double sampleTimeInSec = (double)SAMPLE_TIME_MS / 1000.0;
    g_systemState.kp = kP;
    g_systemState.ki = kI * sampleTimeInSec;
    g_systemState.kd = kD / sampleTimeInSec;
}


void controller_compute() {
    // Erro 
    double e = g_systemState.sp - g_systemState.y;

    // Termo Integral com antiwindup
    g_systemState.iTerm += (g_systemState.ki * e);
    if (g_systemState.iTerm > DAC_RESOLUTION) g_systemState.iTerm = DAC_RESOLUTION;
    else if (g_systemState.iTerm < 0) g_systemState.iTerm = 0;

    // Termo Derivativo
    double dY = g_systemState.y - g_systemState.lastY;

    // Calcula o sinal de controle PID
    g_systemState.u = (g_systemState.kp * e) + g_systemState.iTerm - (g_systemState.kd * dY);

    // Controle de sautracao
    if (g_systemState.u > DAC_RESOLUTION) g_systemState.u = DAC_RESOLUTION;
    else if (g_systemState.u < 0) g_systemState.u = 0;

    // Atualiza a ultma leitura
    g_systemState.lastY = g_systemState.y;
}