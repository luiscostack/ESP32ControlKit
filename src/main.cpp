/**
 * @file main.cpp
 * @brief Ponto de entrada principal do sistema de controle PID com FreeRTOS.
 * Este arquivo é responsável por inicializar o sistema, criar as tarefas de controle,
 * geração de setpoint, plotagem serial e seleção de modo, e então entregar o controle
 * ao escalonador do FreeRTOS.
 */

#include "config.h"
#include "mux.h"
#include "plant.h"
#include "controller.h"
#include "setpoint.h"
#include "web_server.h"
#include "spiffs_defs.h"

// --- Definição das variáveis globais e handles do FreeRTOS ---
// Estas são as definições reais das variáveis declaradas como 'extern' em config.h
SystemState_t g_systemState;
SemaphoreHandle_t xStateMutex;
SemaphoreHandle_t xControlSemaphore;
TimerHandle_t xControlTimer;

// Definição e inicialização real das variáveis de Wi-Fi
const char* WIFI_SSID = "S23 de Luis";
const char* WIFI_PASSWORD = "luis1234";

// --- Protótipos das Tarefas ---
void pid_controller_task(void *parameters);
// void setpoint_generator_task(void *parameters);
void serial_plotter_task(void *parameters);
// void combination_selector_task(void *parameters);
void websocket_plotter_task(void *parameters); 

/**
 * @brief Callback do timer que libera o semáforo para a tarefa de controle.
 * Esta função é executada na frequência definida por SAMPLE_TIME_MS.
 */
void control_timer_callback(TimerHandle_t xTimer) {
    // Libera o semáforo para desbloquear a tarefa de controle (pid_controller_task)
    xSemaphoreGive(xControlSemaphore);
}

void setup() {
    Serial.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(1000)); // Aguarda para o Serial se estabilizar
    Serial.println("--- Inicializando Sistema de Controle PID com FreeRTOS ---");

    // --- SEÇÃO: CONEXÃO WI-FI ---
    Serial.printf("Conectando à rede Wi-Fi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println("\nWi-Fi conectado!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
    // --- FIM DA NOVA SEÇÃO ---

    // 1. Inicialização dos módulos de hardware
    mux_init();
    plant_init();
    
    // 2. Inicialização dos objetos do FreeRTOS
    xStateMutex = xSemaphoreCreateMutex();
    xControlSemaphore = xSemaphoreCreateBinary();
    xControlTimer = xTimerCreate("ControlTimer", pdMS_TO_TICKS(SAMPLE_TIME_MS), pdTRUE, (void *)0, control_timer_callback);

    // INICIALIZAÇÃO DO SPIFFS
    initSPIFFS();

    // INICIALIZAÇÃO DO SERVIDOR WEB
    setup_web_server();

    // Verificação de erros na criação dos objetos RTOS
    if (xStateMutex == NULL || xControlSemaphore == NULL || xControlTimer == NULL) {
        Serial.println("ERRO CRITICO: Falha ao criar objetos do FreeRTOS!");
        while(1); // Trava a execução
    }

    // 3. Inicialização do estado do controlador e do sistema de forma segura
    if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
        controller_init(0.05, 0.1, 0.0);
        g_systemState.active_plant = 1;      // Inicia com a Planta 1
        g_systemState.mux_combination = 0;   // Inicia com a combinação 0
        xSemaphoreGive(xStateMutex);
    }

    // Reporta o estado inicial no terminal
    mux_report_selection(g_systemState.active_plant, g_systemState.mux_combination);

    // 4. Espera para descarga do capacitor
    Serial.println("Aguardando descarga dos capacitores...");
    mux_select_plant(1, 0); // Garante que ambos os DACs estejam em 0
    plant_write_control(1, 0);
    mux_select_plant(2, 0);
    plant_write_control(2, 0);
    vTaskDelay(pdMS_TO_TICKS(TIME_TO_DISCHARGE_MS));

    // 5. Criação das tarefas
    xTaskCreate(pid_controller_task, "PID_Controller_Task", 4096, NULL, 3, NULL);
    // xTaskCreate(setpoint_generator_task, "Setpoint_Generator_Task", 2048, NULL, 2, NULL);
    xTaskCreate(serial_plotter_task, "Serial_Plotter_Task", 2048, NULL, 1, NULL);
    // xTaskCreate(combination_selector_task, "Combination_Selector_Task", 2048, NULL, 1, NULL);
    xTaskCreate(websocket_plotter_task, "WebSocket_Plotter_Task", 3072, NULL, 2, NULL);

    // 6. Inicia o timer que ditará o ritmo do controle
    xTimerStart(xControlTimer, 0);
    
    Serial.println("Sistema iniciado. Tarefas em execução.");
    Serial.println("Setpoint(V),Output(V)"); // Cabeçalho para o Serial Plotter
}

/**
 * @brief Tarefa principal de controle PID. Executa a cada sinal do timer.
 */
void pid_controller_task(void *parameters) {
    for (;;) {
        // Aguarda o sinal do timer (sincronização a cada SAMPLE_TIME_MS)
        if (xSemaphoreTake(xControlSemaphore, portMAX_DELAY) == pdTRUE) {
            int current_plant, current_combination;
            
            // Trava o mutex para obter uma cópia consistente do estado atual
            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
                current_plant = g_systemState.active_plant;
                current_combination = g_systemState.mux_combination;
                xSemaphoreGive(xStateMutex);
            }

            // Atua sobre o hardware com a cópia do estado
            mux_select_plant(current_plant, current_combination);
            vTaskDelay(pdMS_TO_TICKS(1)); // Pequeno delay para estabilização do MUX

            double voltage_y = plant_read_voltage(current_plant);

            // Trava novamente para atualizar o estado e calcular o controle
            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
                g_systemState.y = (voltage_y / VCC) * ADC_RESOLUTION;
                controller_compute();
                xSemaphoreGive(xStateMutex);
            }
            
            // Aplica o sinal de controle na planta (fora da seção crítica)
            plant_write_control(current_plant, g_systemState.u);
        }
    }
}

/**
 * @brief Tarefa para plotar os dados no Serial, compatível com o Serial Plotter.
 */
void serial_plotter_task(void *parameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(150); // Roda a 20Hz

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        double sp_voltage, y_voltage;

        // Usa o Mutex para ler os valores de forma segura
        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
            sp_voltage = (g_systemState.sp / (double)ADC_RESOLUTION) * VCC;
            y_voltage = (g_systemState.y / (double)ADC_RESOLUTION) * VCC;
            xSemaphoreGive(xStateMutex);
        }
        
        Serial.print(sp_voltage, 4); // Imprime com 4 casas decimais para maior precisão
        Serial.print(",");
        Serial.println(y_voltage, 4);
    }
}

/**
 * @brief Tarefa que cicla entre as plantas e suas combinações de MUX.
 * Lógica: Planta 1 (comb 0->1->2->3) -> Planta 2 (comb 0->1) -> Repete
 */
/* void combination_selector_task(void *parameters) {
    for(;;) {
        // Espera um tempo antes de mudar para o próximo estado
        vTaskDelay(pdMS_TO_TICKS(5000)); 

        int new_plant, new_combination;

        // Trava o mutex para modificar o estado do sistema com segurança
        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
            
            // Lógica da máquina de estados para transição
            if (g_systemState.active_plant == 1) {
                // Se estamos na Planta 1
                if (g_systemState.mux_combination < 3) {
                    // Se ainda não chegamos na última combinação, apenas incrementa
                    g_systemState.mux_combination++;
                } else {
                    // Se estávamos na última combinação (3), muda para a Planta 2
                    g_systemState.active_plant = 2;
                    g_systemState.mux_combination = 0; // Começa da primeira combinação da Planta 2
                }
            } else { // if (g_systemState.active_plant == 2)
                // Se estamos na Planta 2
                if (g_systemState.mux_combination < 1) {
                    // Se ainda não chegamos na última combinação, apenas incrementa
                    g_systemState.mux_combination++;
                } else {
                    // Se estávamos na última combinação (1), volta para a Planta 1
                    g_systemState.active_plant = 1;
                    g_systemState.mux_combination = 0; // Começa da primeira combinação da Planta 1
                }
            }
            
            // Copia o novo estado para usar fora da seção crítica
            new_plant = g_systemState.active_plant;
            new_combination = g_systemState.mux_combination;

            xSemaphoreGive(xStateMutex);
        }
        
        // Reporta a nova seleção para o terminal
        mux_report_selection(new_plant, new_combination);
    }
}*/

/**
 * @brief Tarefa que envia dados de telemetria via WebSocket para o gráfico da página web.
 */
void websocket_plotter_task(void *parameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(50); // Envia dados 4 vezes por segundo (4Hz)
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Aguarda pelo próximo ciclo
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Só processa e envia se houver algum cliente web conectado
        if (ws.count() > 0) {
            double sp_local, y_local;

            // 1. Lê os dados do estado global de forma segura
            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
                sp_local = g_systemState.sp;
                y_local = g_systemState.y;
                xSemaphoreGive(xStateMutex);
            }

            // 2. Converte para tensão e prepara o pacote JSON
            double sp_voltage = (sp_local / (double)ADC_RESOLUTION) * VCC;
            double y_voltage = (y_local / (double)ADC_RESOLUTION) * VCC;
            unsigned long current_time = millis();

            // Usamos um documento JSON pequeno e estático para eficiência
            StaticJsonDocument<128> doc;
            doc["time"] = current_time;
            doc["sp_v"] = sp_voltage;
            doc["y_v"] = y_voltage;

            char json_buffer[128];
            serializeJson(doc, json_buffer);

            // 3. Envia a string JSON para todos os clientes conectados
            ws.textAll(json_buffer);
        }
    }
}

void loop() {
    // O loop principal fica vazio, pois o escalonador do FreeRTOS gerencia as tarefas.
    // Deleta a tarefa 'loopTask' do Arduino para economizar recursos.
    vTaskDelete(NULL);
}