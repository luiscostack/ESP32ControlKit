// src/web_server.h

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h> // Inclua para ter a definição de AsyncWebSocket

// Torna o objeto WebSocket visível para outros arquivos, como o main.cpp
extern AsyncWebSocket ws;

void setup_web_server();

#endif // WEB_SERVER_H