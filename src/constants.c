#include "websocket.h"

const char* WS_WEBSOCKET_MAGIC = WS_MAGIC_STRING;
const char* WS_HTTP_UPGRADE_HEADER = "HTTP/1.1 101 Switching Protocols\r\n";
const char* WS_CONNECTION_UPGRADE = "Connection: Upgrade\r\n";
const char* WS_UPGRADE_WEBSOCKET = "Upgrade: websocket\r\n";
const char* WS_SEC_WEBSOCKET_ACCEPT = "Sec-WebSocket-Accept: ";
