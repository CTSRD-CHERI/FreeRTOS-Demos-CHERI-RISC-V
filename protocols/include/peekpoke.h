#include "FreeRTOS_TCP_server.h"
#include "FreeRTOS_server_private.h"

extern size_t peekPokeHandler( HTTPClient_t *pxClient, BaseType_t xIndex, const char *pcURLData, char *pcOutputBuffer, size_t uxBufferLength );
