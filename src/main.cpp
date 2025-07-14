
#include "config.h"
#include "mux.h"
#include "plant.h"
#include "controller.h"
#include "setpoint.h"
#include "web_server.h"
#include "spiffs_defs.h"

// -- variaveis globais e handles do FreeRTOS ---
SystemState_t g_systemState;
SemaphoreHandle_t xStateMutex;
SemaphoreHandle_t xControlSemaphore;
TimerHandle_t xControlTimer;

// Variaveis do Wi-Fi
const char* WIFI_SSID = "S23 de Luis";
const char* WIFI_PASSWORD = "luis1234";

// --- Prototipos das Tarefas ---
void pid_controller_task(void *parameters);
// void setpoint_generator_task(void *parameters);
void serial_plotter_task(void *parameters);
// void combination_selector_task(void *parameters);
void websocket_plotter_task(void *parameters); 

void control_timer_callback(TimerHandle_t xTimer) {
    // Libera o semáforo para desbloquear a tarefa de controle (pid_controller_task)
    xSemaphoreGive(xControlSemaphore);
}

void setup() {
    Serial.begin(115200);
    vTaskDelay(pdMS_TO_TICKS(1000)); // Aguarda para a Serial se estabilizar
    Serial.println("--- Inicializando Sistema de Controle PID com FreeRTOS ---");

    // ---CONEXAO WI-FI ---
    Serial.printf("Conectando à rede Wi-Fi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }
    Serial.println("\nWi-Fi conectado!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());

    // Inicializacao dos modulos de hardware
    mux_init();
    plant_init();
    
    // Inicializacao dos objetos do FreeRTOS
    xStateMutex = xSemaphoreCreateMutex();
    xControlSemaphore = xSemaphoreCreateBinary();
    xControlTimer = xTimerCreate("ControlTimer", pdMS_TO_TICKS(SAMPLE_TIME_MS), pdTRUE, (void *)0, control_timer_callback);

    // INICIALIZAÇAO DO SPIFFS
    initSPIFFS();

    // INICIALIZAÇAO DO SERVIDOR WEB
    setup_web_server();

    // Verificacao de erros na criação dos objetos RTOS
    if (xStateMutex == NULL || xControlSemaphore == NULL || xControlTimer == NULL) {
        Serial.println("ERRO CRITICO: Falha ao criar objetos do FreeRTOS!");
        while(1); // Trava a execucao
    }

    // Inicializacao do estado do controlador e do sistema de forma segura
    if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
        controller_init(0.05, 0.1, 0.0);
        g_systemState.active_plant = 1;      // Inicia com a Planta 1
        g_systemState.mux_combination = 0;   // Inicia com a combinação 0
        xSemaphoreGive(xStateMutex);
    }

    // Reporta o estado inicial no terminal
    mux_report_selection(g_systemState.active_plant, g_systemState.mux_combination);

    // Espera para descarga do capacitor
    Serial.println("Aguardando descarga dos capacitores...");
    mux_select_plant(1, 0); // Garante que os DACs estejam em 0
    plant_write_control(1, 0);
    mux_select_plant(2, 0);
    plant_write_control(2, 0);
    vTaskDelay(pdMS_TO_TICKS(TIME_TO_DISCHARGE_MS));

    // Criacao das tarefas
    xTaskCreate(pid_controller_task, "PID_Controller_Task", 4096, NULL, 3, NULL);
    // xTaskCreate(setpoint_generator_task, "Setpoint_Generator_Task", 2048, NULL, 2, NULL);
    xTaskCreate(serial_plotter_task, "Serial_Plotter_Task", 2048, NULL, 1, NULL);
    // xTaskCreate(combination_selector_task, "Combination_Selector_Task", 2048, NULL, 1, NULL);
    xTaskCreate(websocket_plotter_task, "WebSocket_Plotter_Task", 3072, NULL, 2, NULL);

    // Inicia o timer
    xTimerStart(xControlTimer, 0);
    
    Serial.println("Sistema iniciado. Tarefas em execução.");
    Serial.println("Setpoint(V),Output(V)"); // Cabeçalho para o Serial Plotter
}

void pid_controller_task(void *parameters) {
    for (;;) {
        // Aguarda o sinal do timer
        if (xSemaphoreTake(xControlSemaphore, portMAX_DELAY) == pdTRUE) {
            int current_plant, current_combination;
            
            // obter estado atual
            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
                current_plant = g_systemState.active_plant;
                current_combination = g_systemState.mux_combination;
                xSemaphoreGive(xStateMutex);
            }

            // ativa a planta e combinaçao atual
            mux_select_plant(current_plant, current_combination);
            vTaskDelay(pdMS_TO_TICKS(1));

            double voltage_y = plant_read_voltage(current_plant);

            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
                g_systemState.y = (voltage_y / VCC) * ADC_RESOLUTION;
                controller_compute();
                xSemaphoreGive(xStateMutex);
            }
            
            // Aplica o sinal de controle
            plant_write_control(current_plant, g_systemState.u);
        }
    }
}

void serial_plotter_task(void *parameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(150);

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        double sp_voltage, y_voltage;

        // Usa o Mutex para ler os valores de forma segura
        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
            sp_voltage = (g_systemState.sp / (double)ADC_RESOLUTION) * VCC;
            y_voltage = (g_systemState.y / (double)ADC_RESOLUTION) * VCC;
            xSemaphoreGive(xStateMutex);
        }
        
        Serial.print(sp_voltage, 4); // Imprime com 4 casas decimais para maior precisao
        Serial.print(",");
        Serial.println(y_voltage, 4);
    }
}

/* void combination_selector_task(void *parameters) {
    for(;;) {
        // Espera um tempo antes de mudar para o proximo estado
        vTaskDelay(pdMS_TO_TICKS(5000)); 

        int new_plant, new_combination;

        // Trava o mutex para modificar o estado do sistema com segurança
        if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
            
            // Logica da maquina de estados para transicaoo
            if (g_systemState.active_plant == 1) {
                // Se estamos na Planta 1
                if (g_systemState.mux_combination < 3) {
                    g_systemState.mux_combination++;
                } else {
                    g_systemState.active_plant = 2;
                    g_systemState.mux_combination = 0; 
                }
            } else { // if (g_systemState.active_plant == 2)
                if (g_systemState.mux_combination < 1) {
                    g_systemState.mux_combination++;
                } else {
                    g_systemState.active_plant = 1;
                    g_systemState.mux_combination = 0; // Começa da primeira combinação da Planta 1
                }
            }
            
            new_plant = g_systemState.active_plant;
            new_combination = g_systemState.mux_combination;

            xSemaphoreGive(xStateMutex);
        }
 
        mux_report_selection(new_plant, new_combination);
    }
}*/

void websocket_plotter_task(void *parameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(250); // Envia dados 4 vezes por segundo
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // Aguarda o proximo ciclo
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        ws.cleanupClients();

        // So envia se tiver cliente web conectado
        if (ws.count() > 0) {
            double sp_local, y_local;

            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
                sp_local = g_systemState.sp;
                y_local = g_systemState.y;
                xSemaphoreGive(xStateMutex);
            }

            // MOnta pacote JSON
            double sp_voltage = (sp_local / (double)ADC_RESOLUTION) * VCC;
            double y_voltage = (y_local / (double)ADC_RESOLUTION) * VCC;
            unsigned long current_time = millis();

            StaticJsonDocument<128> doc;
            doc["time"] = current_time;
            doc["sp_v"] = sp_voltage;
            doc["y_v"] = y_voltage;

            char json_buffer[128];
            serializeJson(doc, json_buffer);

            // 3. Envia o JSON para os clientes
            ws.textAll(json_buffer);
        }
    }
}

void loop() {
    vTaskDelete(NULL);
}