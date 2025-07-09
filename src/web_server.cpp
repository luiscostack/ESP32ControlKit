// src/web_server.cpp

#include "web_server.h"
#include "config.h"
#include "controller.h" // Para usar controller_set_tunings
#include "mux.h"        // Para usar mux_report_selection

// --- Definição dos objetos do servidor ---
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

/**
 * @brief Função chamada quando um evento de WebSocket ocorre.
 */
void on_web_socket_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("Cliente WebSocket conectado: #%u\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("Cliente WebSocket desconectado: #%u\n", client->id());
    } else if (type == WS_EVT_DATA) {
        // Um pacote de dados foi recebido
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            
            // 1. Interpretar os dados recebidos como JSON
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, (char*)data, len);

            if (error) {
                Serial.print("Falha ao interpretar JSON: ");
                Serial.println(error.c_str());
                return;
            }

            // 2. Extrair os valores do JSON
            double kp = doc["kp"];
            double ki = doc["ki"];
            double kd = doc["kd"];
            double sp = doc["setpoint"];
            int planta = doc["planta"];
            int combinacao = doc["combinacao"];

            Serial.println("Recebido novo JSON com parâmetros:");
            Serial.printf("  kp=%.3f, ki=%.3f, kd=%.3f, sp=%.2f\n", kp, ki, kd, sp);
            Serial.printf("  planta=%d, combinacao=%d\n", planta, combinacao);

            // 3. Atualizar o estado do sistema de forma segura
            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
                // Atualiza os ganhos do PID usando a função apropriada
                controller_set_tunings(kp, ki, kd);

                // Atualiza os outros parâmetros diretamente
                g_systemState.sp = sp;
                g_systemState.active_plant = planta;
                g_systemState.mux_combination = combinacao;

                xSemaphoreGive(xStateMutex);
            }
            
            // 4. Reporta a mudança no terminal para feedback imediato
            mux_report_selection(planta, combinacao);
        }
    }
}

/**
 * @brief Configura o servidor web e o WebSocket.
 */
void setup_web_server() {
    // Anexa o manipulador de eventos ao WebSocket
    ws.onEvent(on_web_socket_event);
    server.addHandler(&ws);

    // Inicia o servidor
    server.begin();
    Serial.println("Servidor Web e WebSocket iniciados.");
}