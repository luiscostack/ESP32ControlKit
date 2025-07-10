// src/web_server.h

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h> // nao funciona
//#include <config.h>
//#include <AsyncTCP.h>
// Torna o objeto WebSocket visível para outros arquivos, como o main.cpp
extern AsyncWebSocket ws;

void on_web_socket_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void setup_web_server();

#endif // WEB_SERVER_H