// src/web_server.h

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h> 
//#include <config.h>
//#include <AsyncTCP.h>

extern AsyncWebSocket ws;

void on_web_socket_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void setup_web_server();

#endif // WEB_SERVER_H