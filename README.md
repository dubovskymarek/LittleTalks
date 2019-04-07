<b>LittleTalks</b>
<p>
LittleTalks is very tiny communication library based on UDP and it's inspired by MQTT protocol,
but LittleTalks is designed as P2P communication in local network for support connection 64 devices at the same time.<br/>
This library is portable written in c language, at beginning it was designed for microprocessors with low performance and low memory, but it supports desktop and mobile platforms.
Library centralises all UDP implementation in LTPlatformAdapter.c file, that can be overriden to optional platform.
<br/>
LittleTalks was created for startup canny.tech, where was used on both sides (application and device).
</p>

<b>Building from source</b><br/>
Just run make to build.<br/>
You can use QT to build too (look examples/example1/Example1.pro)

<b>Documentation</b><br/>
docs/html/index.html<br/>

<b>Credits</b><br/>
LittleTalks was written by Marek Dubovsky dubovskymarek@icloud.com<br/>

<b>Quick start</b>
<p>Example1 demonstrates basic sending and receiving 2 topics between 2 or more users.</p>

```c
#include "LittleTalks.h"

#define STR_MAX_LENGTH                 50
#define VARIABLE_NUMBER_TOPIC_ID       0x01
#define VARIABLE_STRING_TOPIC_ID       0x02

LT_DEVICE_ID g_thisDeviceId = 0;
BOOL g_thisDeviceIsSender = TRUE;

void OnConnect(LT_DEVICE_ID deviceId)
{
    printf("Connect device %llu\n", deviceId);
    fflush(stdout);
    
    if(g_thisDeviceId < deviceId)
    {
        printf("I'm receiver\n");
        fflush(stdout);
        g_thisDeviceIsSender = FALSE;
    }
}
void OnDisconnect(LT_DEVICE_ID deviceId)
{
    printf("Disconnect device %llu\n", deviceId);
    fflush(stdout);
    
    if(g_thisDeviceId < deviceId)
    {
        g_thisDeviceIsSender = TRUE;
        printf("I'm sender\n");
        fflush(stdout);
    }
}
void OnSubscribe(LT_DEVICE_ID deviceId, LT_UINT64 topicId)
{
    if(topicId == VARIABLE_NUMBER_TOPIC_ID)
    {
        printf("Subscribe variable number from device %llu\n", deviceId);
        fflush(stdout);
    }
    else if(topicId == VARIABLE_STRING_TOPIC_ID)
    {
        printf("Subscribe variable string from device %llu\n", deviceId);
        fflush(stdout);
    }
}
void OnReceive(LT_DEVICE_ID deviceId, LT_UINT64 topicId, BYTE* value, int valueSize)
{
    UNUSED(valueSize);
    if(topicId == VARIABLE_NUMBER_TOPIC_ID)
    {
        printf("Receive variable number %d from device %llu\n", (int)*value, deviceId);
        fflush(stdout);
    }
    else if(topicId == VARIABLE_STRING_TOPIC_ID)
    {
        printf("Receive variable string \"%s\" from device %llu\n", (char*)value,  deviceId);
        fflush(stdout);
    }
}
int main(int argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    
    printf("Example1\n");
    fflush(stdout);
    
    g_thisDeviceId = (LT_DEVICE_ID)(LTPlatformAdapter_GetIP() % 256);
    
    LT_Init(g_thisDeviceId, OnConnect, OnDisconnect, OnSubscribe, OnReceive);
    LT_Subscribe(VARIABLE_NUMBER_TOPIC_ID, sizeof(int));
    LT_Subscribe(VARIABLE_STRING_TOPIC_ID, STR_MAX_LENGTH);
    LT_Start();
    
    printf("This device id: %llu\n", g_thisDeviceId);
    fflush(stdout);
    
    printf("I'm sender\n");
    fflush(stdout);
    
    int iter = 0;
    while(TRUE)
    {
        if(g_thisDeviceIsSender)
        {
            if(iter % 10 != 0)
            {
                LT_Publish(VARIABLE_NUMBER_TOPIC_ID, (BYTE*)&iter, sizeof(int));
            }
            else
            {
                char str[STR_MAX_LENGTH];
                sprintf(str, "completed %d", iter);
                LT_Publish(VARIABLE_STRING_TOPIC_ID, (BYTE*)str, (LT_UINT16)(strlen(str) + 1));
            }
        }
        
        LT_MICROSLEEP(1000000);
        iter++;
    }
    
    LT_Uninit();
    return 0;
}
```
