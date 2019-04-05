#include "LittleTalks.h"

extern struct LittleTalks* g_littleTalks;

BOOL LT_IsInit()
{
    return g_littleTalks != NULL;
}

BOOL LT_Init(LT_DEVICE_ID deviceId,
             LT_OnConnect_Func onConnected_funcPtr,
             LT_OnDisconnect_Func onDisconnected_funcPtr,
             LT_OnSubscribed_Func onSubscribed_funcPtr,
             LT_OnReceive_Func onReceive_funcPtr)
{
    g_littleTalks = (struct LittleTalks*)LT_MALLOC(sizeof(struct LittleTalks));

    if(g_littleTalks == NULL)
    {
        LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);
        return FALSE;
    }

    memset(g_littleTalks, 0, sizeof(struct LittleTalks));

    g_littleTalks->myDevice = (struct LTDevice*)LT_MALLOC(sizeof(struct LTDevice));
    if(g_littleTalks->myDevice == NULL)
    {
        LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);

        LT_FREE(g_littleTalks);
        g_littleTalks = NULL;
        return FALSE;
    }

    g_littleTalks->topicsList = (struct LTTopic*)LT_MALLOC(sizeof(struct LTTopic));
    if(g_littleTalks->topicsList == NULL)
    {
        LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);

        LT_FREE(g_littleTalks->myDevice);
        g_littleTalks->myDevice = NULL;
        LT_FREE(g_littleTalks);
        g_littleTalks = NULL;
        return FALSE;
    }

    g_littleTalks->majorVersion = LT_MAJOR_VERSION;
    g_littleTalks->minorVersion = LT_MINOR_VERSION;

    g_littleTalks->onConnected_funcPtr = onConnected_funcPtr;
    g_littleTalks->onDisconnected_funcPtr = onDisconnected_funcPtr;
    g_littleTalks->onSubscribed_funcPtr = onSubscribed_funcPtr;
    g_littleTalks->onReceive_funcPtr = onReceive_funcPtr;

    g_littleTalks->isRunning = FALSE;

    LTDevice_Init(g_littleTalks->myDevice, deviceId, LTPlatformAdapter_GetIP());
    g_littleTalks->otherDeviceList = g_littleTalks->myDevice;

    g_littleTalks->udpSocket = NULL;

    g_littleTalks->tick = 0;
    g_littleTalks->devicesCount = 0;
    g_littleTalks->sendIntroductionCounter = 0;

    LTTopic_Init(g_littleTalks->topicsList, 0, 0);

    LTPlatformAdapter_Init(LT_OnIncomingUdpPacket);

    return TRUE;
}
void LT_Uninit()
{
    struct LTDevice* deviceIter;

    if(g_littleTalks == NULL)
        return;

    if(g_littleTalks->isRunning)
        LT_Stop();

    deviceIter = g_littleTalks->otherDeviceList;
    while(NULL != (deviceIter = LTDeviceList_Next(deviceIter)))
    {
        if(deviceIter->isConnected)
        {
            deviceIter->autoRelease = FALSE;
            LTDevice_SetDisconnected(deviceIter);
        }
    }

    LTPlatformAdapter_Uninit();

    LTDeviceList_Clear(g_littleTalks->otherDeviceList);
    LTDevice_Uninit(g_littleTalks->myDevice);
    LT_FREE(g_littleTalks->myDevice);
    g_littleTalks->myDevice = NULL;
    g_littleTalks->otherDeviceList = NULL;

    LTTopicList_Clear(g_littleTalks->topicsList);
    LT_FREE(g_littleTalks->topicsList);
    g_littleTalks->topicsList = NULL;
    g_littleTalks->sendIntroductionCounter = 0;

    g_littleTalks->onConnected_funcPtr = NULL;
    g_littleTalks->onDisconnected_funcPtr = NULL;
    g_littleTalks->onSubscribed_funcPtr = NULL;
    g_littleTalks->onReceive_funcPtr = NULL;

    LT_FREE(g_littleTalks);
    g_littleTalks = NULL;
}

void LT_Start()
{
    if(g_littleTalks->isRunning)
        return;

    g_littleTalks->isRunning = TRUE;
    LTPlatformAdapter_Start();
}
void LT_Stop()
{
    LT_UINT32 bcastIp;

    if(!g_littleTalks->isRunning)
        return;

    bcastIp = LTPlatformAdapter_GetGateway() | ((~LTPlatformAdapter_GetNetworkMask()) & 0xffffffff);
    LT_SendDisconnect(bcastIp);

    g_littleTalks->isRunning = FALSE;

    LTPlatformAdapter_Stop();
}

void LT_Publish(LT_TOPIC_ID topicId, BYTE* value, LT_UINT16 valueSize)
{
    struct LTTopic* topic;

    if(g_littleTalks == NULL)
        return;

    LTPlatformAdapter_Lock();

    topic = LT_FoundTopic(topicId);
    if(topic == NULL)
    {
        LTPlatformAdapter_Unlock();
        return;
    }

    topic->modifyCounter++;

    LT_SendTopic(topic, topic->devicesFlag, value, valueSize);
    topic->isChanged = TRUE;

    LTPlatformAdapter_Unlock();
}

BOOL LT_Subscribe(LT_TOPIC_ID topicId, LT_UINT16 valueSizeMax)
{
    struct LTTopic* foundTopic;
    struct LTTopic* newTopic;

    if(g_littleTalks == NULL)
        return FALSE;

    if(LT_UDP_PACKET_SIZE_MAX < valueSizeMax)
    {
        LT_DetectError(LTError_MaxSizeOfUdpPacketOverflow, LT_CURR_CODE_LINE);
        return FALSE;
    }

    LTPlatformAdapter_Lock();

    foundTopic = LT_FoundTopic(topicId);
    if(foundTopic != NULL)
    {
        LTPlatformAdapter_Unlock();

        return FALSE;
    }

    newTopic = (struct LTTopic*)LT_MALLOC(sizeof(struct LTTopic) + valueSizeMax);
    if(newTopic == NULL)
    {
        LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);

        LTPlatformAdapter_Unlock();

        return FALSE;
    }

    LTTopic_Init(newTopic, topicId, valueSizeMax);
    LTTopicList_Push(g_littleTalks->topicsList, newTopic);

    LTPlatformAdapter_Unlock();

    return TRUE;
}

BOOL LT_ForceSubscribeRemoteTopic(LT_DEVICE_ID deviceId, LT_TOPIC_ID topicId)
{
    int j;
    int deviceIndex;
    LT_DEVICE_FLAG devicesFlag;

    struct LTTopic* topic;
    struct LTDevice* device;

    if(g_littleTalks == NULL)
        return FALSE;

    LTPlatformAdapter_Lock();

    topic = LT_FoundTopic(topicId);
    if(topic == NULL)
    {
        LTPlatformAdapter_Unlock();

        return FALSE;
    }

    device = LTDevice_SetConnected(deviceId);
    if(device == NULL)
    {
        LTPlatformAdapter_Unlock();

        return FALSE;
    }

    deviceIndex = LTDeviceList_IndexOf(g_littleTalks->otherDeviceList, device);
    devicesFlag = 1;
    for(j = 0; j < deviceIndex; j++)
        devicesFlag *= 2;

    if(!(topic->devicesFlag & devicesFlag))
    {
        topic->devicesFlag = topic->devicesFlag | devicesFlag;

        g_littleTalks->onSubscribed_funcPtr(deviceId, topicId);
    }

    LTPlatformAdapter_Unlock();

    return TRUE;
}

void LT_AddDevice(LT_DEVICE_ID deviceId, BOOL toExpectLocal)
{
    struct LTDevice* device;

    LTPlatformAdapter_Lock();

    if(g_littleTalks->devicesCount == LT_DEVICES_MAX_COUNT)
    {
        LT_DetectError(LTError_MaxDevicesCount, LT_CURR_CODE_LINE);

        LTPlatformAdapter_Unlock();

        return;
    }

    device = (struct LTDevice*)LT_MALLOC(sizeof(struct LTDevice));
    if(device == NULL)
    {
        LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);

        LTPlatformAdapter_Unlock();

        return;
    }

    LTDevice_Init(device, deviceId, 0);
    device->autoRelease = FALSE;
    device->toExpectLocal = toExpectLocal;

    LTDeviceList_Push(g_littleTalks->otherDeviceList, device);
    g_littleTalks->devicesCount++;

    LTPlatformAdapter_Unlock();
}

BOOL LT_IsSomeDeviceRemoted(LT_DEVICE_FLAG deviceFlag)
{
    LT_DEVICE_FLAG devFlag;
    struct LTDevice* deviceIter;

    devFlag = 1;
    deviceIter = g_littleTalks->otherDeviceList;
    while(NULL != (deviceIter = LTDeviceList_Next(deviceIter)))
    {
        if(deviceFlag & devFlag)
            if(deviceIter->isConnected && deviceIter->isRemoteConnected)
                return TRUE;

        devFlag *= 2;
    }

    return FALSE;
}

BOOL LT_OnStartMainLoop()
{
    if(g_littleTalks == NULL)
        return FALSE;

#ifndef ESP_PLATF
    LTPlatformAdapter_Lock();
#endif

    g_littleTalks->udpSocket = (struct LTUdpSocket*)LT_MALLOC(sizeof(struct LTUdpSocket));
    if(g_littleTalks->udpSocket == NULL)
    {
        LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);

#ifndef ESP_PLATF
        LTPlatformAdapter_Unlock();
#endif
        return FALSE;
    }

    LTPlatformAdapter_UDP_Init(g_littleTalks->udpSocket);

    LTPlatformAdapter_UDP_Bind(g_littleTalks->udpSocket, LT_UDP_PORT);

#ifndef ESP_PLATF
    LTPlatformAdapter_Unlock();
#endif
    return TRUE;
}

void LT_OnStepMainLoop()
{
    LT_UINT32 tickSeconds;
    LT_DEVICE_FLAG devFlag;
    struct LTDevice* deviceIter;
    struct LTTopic* topicIter;
    BOOL isConnected;

#ifndef ESP_PLATF
    LTPlatformAdapter_Lock();
#endif

    topicIter = g_littleTalks->topicsList;
    while(NULL != (topicIter = LTTopicList_Next(topicIter)))
    {
        if(topicIter->publishDevicesFlag == 0)
            continue;

        if(g_littleTalks->tick == 0)
        {
            topicIter->publishedTick = 0;
            continue;
        }

        if(g_littleTalks->tick - topicIter->publishedTick < LT_CONFIRMATION_DELAY * LT_STEP_FREQUENCE)
            continue;

        devFlag = topicIter->publishDevicesFlag;
        topicIter->publishDevicesFlag = 0;

        LT_SendTopic(topicIter, devFlag, (BYTE*)(topicIter + 1), topicIter->valueSize);
    }

    if(g_littleTalks->tick % LT_STEP_FREQUENCE != 0)
    {
        g_littleTalks->tick++;
        if(0x8fffffff < g_littleTalks->tick)
            g_littleTalks->tick = 0;

#ifndef ESP_PLATF
        LTPlatformAdapter_Unlock();
#endif
        return;
    }

    g_littleTalks->tick++;
    if(0x8fffffff < g_littleTalks->tick)
        g_littleTalks->tick = 0;

    tickSeconds = g_littleTalks->tick / LT_STEP_FREQUENCE;

    deviceIter = g_littleTalks->otherDeviceList;
    while(NULL != (deviceIter = LTDeviceList_Next(deviceIter)))
    {
        if(deviceIter->isConnected)
        {
            isConnected = FALSE;
            if(deviceIter->isLocalConnected)
            {
                if(deviceIter->localReceiveCounter < LT_LOCAL_KEEPALIVE_TIMEOUT_MAX)
                {
                    isConnected = TRUE;
                    deviceIter->localReceiveCounter++;
                }
                else
                    deviceIter->isLocalConnected = FALSE;
            }

            if(deviceIter->isRemoteConnected)
            {
                if(deviceIter->remoteReceiveCounter < LT_REMOTE_KEEPALIVE_TIMEOUT_MAX)
                {
                    isConnected = TRUE;
                    deviceIter->remoteReceiveCounter++;
                }
                else
                    deviceIter->isRemoteConnected = FALSE;
            }

            if(!isConnected)
            {
                LTDevice_SetDisconnected(deviceIter);

                continue;
            }
        }

        if(deviceIter->waitingForResponse)
        {
            if(deviceIter->waitingForResponseCounter < 10)
            {
                ///First 10 seconds: send request every 1 second
                LT_SendRequestTopics(deviceIter);
            }
            else if(deviceIter->waitingForResponseCounter < 40)
            {
                ///10-40 seconds: send request every 5 seconds
                if(tickSeconds % 5 == 0)
                    LT_SendRequestTopics(deviceIter);
            }
            else
            {
                ///40-infinity seconds: send request every 30 seconds
                if(tickSeconds % 30 == 0)
                    LT_SendRequestTopics(deviceIter);
            }

            if(deviceIter->waitingForResponseCounter < 255)
                deviceIter->waitingForResponseCounter++;
        }
    }

    if(((tickSeconds + LT_BROADCAST_KEEPALIVE_AND_INTRODUCTION_INTERVAL - 1) % LT_BROADCAST_KEEPALIVE_AND_INTRODUCTION_INTERVAL) == 0)
        LT_SendIntroductionBroadcast();

    LTPlatformAdapter_Unlock();
}
void LT_OnEndMainLoop()
{
    LTPlatformAdapter_UDP_Uninit(g_littleTalks->udpSocket);
    LT_FREE(g_littleTalks->udpSocket);
    g_littleTalks->udpSocket = NULL;
}

