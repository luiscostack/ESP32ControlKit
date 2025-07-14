// src/config.h

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
#include <driver/dac.h>
#include <WiFi.h>
#include <AsyncTCP.h> // sem isso, ESPAsyncWebServer nao funciona
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "spiffs_defs.h" // Para usar initSPIFFS


extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// --- Definições de Hardware e Pinos ---
const double VCC = 3.3; // Tensão de operação do ESP32
const uint16_t ADC_RESOLUTION = 4095; // Resolução do ADC (0-4095 para 12 bits)
const uint8_t DAC_RESOLUTION = 255;  // Resolução do DAC (0-255 para 8 bits)

// Pinos para a Planta 1
const int ADC_PIN_PLANT_1 = 34;
const int DAC_PIN_PLANT_1 = 25; // DAC canal 1

// Pinos para a Planta 2 
const int ADC_PIN_PLANT_2 = 35;
const int DAC_PIN_PLANT_2 = 26; // DAC Channel 2

// Pinos de controle do mux
const int MUX_IN_A_PIN = 32;
const int MUX_IN_B_PIN = 33;

// --- Configurações do Sistema de Controle ---
const unsigned long SAMPLE_TIME_MS = 200; // Tempo de amostragem
const unsigned long TIME_TO_DISCHARGE_MS = 8000; //

// --- Definições do FreeRTOS ---

// Estrutura para armazenar o estado compartilhado do sistema
typedef struct {
    double u;          // Sinal de controle
    double y;          // Saída da planta 
    double sp;         // Referenciah
    double iTerm;      // Termo integral acumulado
    double lastY;      // Ultima leitura da saida
    double kp, ki, kd; // Ganhos do PID

    // Campos para gerenciar o estado completo do sistema
    int active_plant;      // Guarda o ID da planta ativa (1 ou 2)
    int mux_combination;   // Guarda a combinação do MUX (0-3 para Planta 1, 0-1 para Planta 2)

} SystemState_t;

// Mutex para proteger o acesso a estrutura de estado compartilhado
extern SemaphoreHandle_t xStateMutex;

// Semaphore para sincronizar o loop de controle
extern SemaphoreHandle_t xControlSemaphore;

// Timer para disparar o controle periodicamente
extern TimerHandle_t xControlTimer;

// Declaracaoo da variavel de estado global
extern SystemState_t g_systemState;

#endif // CONFIG_H