//! [Example 1]

#include "LittleTalks.h"

#define STR_MAX_LENGTH               50
#define VARIABLE_NUMBER_TOPIC_ID       0x01
#define VARIABLE_STRING_TOPIC_ID       0x02

void OnConnect(LT_DEVICE_ID deviceId)
{
    printf("Connect device %llu\n", deviceId);
    fflush(stdout);
}
void OnDisconnect(LT_DEVICE_ID deviceId)
{
    printf("Disconnect device %llu\n", deviceId);
    fflush(stdout);
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
    LT_DEVICE_ID thisDeviceUniqueId = (LT_DEVICE_ID)(LTPlatformAdapter_GetIP() % 256);

    LT_Init(thisDeviceUniqueId, OnConnect, OnDisconnect, OnSubscribe, OnReceive);

    LT_Subscribe(VARIABLE_NUMBER_TOPIC_ID, sizeof(int));
    LT_Subscribe(VARIABLE_STRING_TOPIC_ID, STR_MAX_LENGTH);

    LT_Start();

    printf("This device id: %llu\n", thisDeviceUniqueId);
    fflush(stdout);

    unsigned int randomPeriodMs = 1000 + thisDeviceUniqueId % 1000;
    srand(randomPeriodMs);

    int iter = 0;
    ///Infinity loop
    while(iter < 10000000)
    {
        int n = iter % 5;

        /// Publish number
        if(n == 0 || n == 1)
        {
            int variableNum = rand() % 10;

            LT_Publish(VARIABLE_NUMBER_TOPIC_ID, (BYTE*)&variableNum, sizeof(int));
        }

        /// Publish string
        else if(n == 2)
        {
            char str[STR_MAX_LENGTH];
            sprintf(str, "abcdefg %d", rand() % 10);
            LT_Publish(VARIABLE_STRING_TOPIC_ID, (BYTE*)str, (LT_UINT16)(strlen(str) + 1));
        }
        else if(n == 3)
        {
            char str[STR_MAX_LENGTH];
            sprintf(str, "XYZ %d", rand() % 10);
            LT_Publish(VARIABLE_STRING_TOPIC_ID, (BYTE*)str, (LT_UINT16)(strlen(str) + 1));
        }

        LT_MICROSLEEP(randomPeriodMs * 1000);

        iter++;
    }

    LT_Uninit();

    return 0;
}

//! [Example 1]
