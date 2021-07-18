#ifndef _IPC_TYPES_H
#define _IPC_TYPES_H

typedef enum ipcType
{
    QUEUES = 0,
    NOTIFICATIONS = 1,
    ALL
} IPCType_t;
/*-----------------------------------------------------------*/

typedef struct taskParams
{
    UBaseType_t xBufferSize;
    UBaseType_t xTotalSize;
    IPCType_t xIPCMode;
    QueueHandle_t* xQueue;
    TaskHandle_t senderTask;
    TaskHandle_t receiverTask;
    TaskHandle_t mainTask;
} IPCTaskParams_t;
/*-----------------------------------------------------------*/
#endif
