// src/web_server.cpp - VERSÃO COM VALIDAÇÃO DE PONTO FLUTUANTE CORRIGIDA

#include "web_server.h"
#include "config.h"
#include "controller.h"
#include "mux.h"

// Definição dos objetos do servidor
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void on_web_socket_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("Cliente WebSocket conectado: #%u\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("Cliente WebSocket desconectado: #%u\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, (char*)data, len);

            if (error) {
                Serial.print("Falha ao interpretar JSON: ");
                Serial.println(error.c_str());
                return;
            }

            if (xSemaphoreTake(xStateMutex, portMAX_DELAY) == pdTRUE) {
                // Atualiza ganhos, planta, etc. (lógica existente)
                if (doc.containsKey("kp") && doc.containsKey("ki") && doc.containsKey("kd")) {
                    controller_set_tunings(doc["kp"], doc["ki"], doc["kd"]);
                    Serial.printf("Ganhos PID atualizados: Kp=%.3f, Ki=%.3f, Kd=%.3f\n", (double)doc["kp"], (double)doc["ki"], (double)doc["kd"]);
                }

                if (doc.containsKey("planta") && doc.containsKey("combinacao")) {
                    g_systemState.active_plant = doc["planta"];
                    g_systemState.mux_combination = doc["combinacao"];
                    mux_report_selection(g_systemState.active_plant, g_systemState.mux_combination);
                }

                if (doc.containsKey("setpoint_v")) {
                    double received_sp_voltage = doc["setpoint_v"];

                    // Passo 1: Clamping (Saturação) do valor recebido
                    // Garante que o valor usado para o cálculo nunca seja maior que VCC ou menor que 0
                    if (received_sp_voltage > VCC) {
                        received_sp_voltage = VCC;
                    } else if (received_sp_voltage < 0.0) {
                        received_sp_voltage = 0.0;
                    }

                    // Passo 2: Conversão e atualização com o valor já validado e "clampado"
                    g_systemState.sp = (received_sp_voltage / VCC) * ADC_RESOLUTION;
                    
                    Serial.printf("Setpoint recebido: %.4f V -> Valor usado: %.4f V -> Convertido para: %.0f\n", 
                                  (double)doc["setpoint_v"], received_sp_voltage, g_systemState.sp);
                }
                
                xSemaphoreGive(xStateMutex);
            }
        }
    }
}

void setup_web_server() {
    ws.onEvent(on_web_socket_event);
    server.addHandler(&ws);
    server.begin();

     // --- Configuração das rotas do Web Service ---

    // Serve os arquivos estáticos diretamente da raiz do SPIFFS.
    // O primeiro argumento é a rota na URL.
    // O segundo é o sistema de arquivos (SPIFFS).
    // O terceiro é o caminho do arquivo no SPIFFS.

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    Serial.println("Servidor Web e WebSocket iniciados.");
}