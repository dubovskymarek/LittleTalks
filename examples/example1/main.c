//! [Example 1]

#include "LittleTalks.h"

#define STR_MAX_LENGTH               50
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

    ///Generate unique id by last byte from IP address
    g_thisDeviceId = (LT_DEVICE_ID)(LTPlatformAdapter_GetIP() % 256);

    LT_Init(g_thisDeviceId, OnConnect, OnDisconnect, OnSubscribe, OnReceive);

    LT_Subscribe(VARIABLE_NUMBER_TOPIC_ID, sizeof(int));
    LT_Subscribe(VARIABLE_STRING_TOPIC_ID, STR_MAX_LENGTH);

    LT_Start();

    printf("This device id: %llu\n", g_thisDeviceId);
    fflush(stdout);

    printf("I'm sender\n");
    fflush(stdout);

    ///Infinity loop
    int iter = 0;
    while(TRUE)
    {
        if(g_thisDeviceIsSender)
        {
            /// Publish number
            if(iter % 10 != 0)
            {
                LT_Publish(VARIABLE_NUMBER_TOPIC_ID, (BYTE*)&iter, sizeof(int));
            }

            /// Publish string
            else
            {
                char str[STR_MAX_LENGTH];
                sprintf(str, "completed %d", iter);
                LT_Publish(VARIABLE_STRING_TOPIC_ID, (BYTE*)str, (LT_UINT16)(strlen(str) + 1));
            }
        }

        ///Sleep 1 second
        LT_MICROSLEEP(1000000);

        iter++;
    }

    LT_Uninit();

    return 0;
}

//! [Example 1]
