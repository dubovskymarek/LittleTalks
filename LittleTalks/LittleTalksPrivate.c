#include "LittleTalksPrivate.h"

struct LittleTalks* g_littleTalks = NULL;

void LT_OnIncomingUdpPacket(BYTE* data, int dataSize)
{
    LT_ProcessReceivedData(data, dataSize);
}

void LT_ProcessReceivedData(BYTE* data, int dataSize)
{
    LT_UINT32 deviceIp;
    struct LTDevice* device;
    enum LTPacketType packetType;
    LT_TOPIC_ID* reqTopics;
    LT_UINT16 reqTopicsCount;
    int contentDataSize;

    while(LT_PACKET_HEADER_SIZE <= dataSize)
    {
        if(!LT_CheckPacket(data, dataSize))
            break;

        packetType = (enum LTPacketType)data[6];

        memcpy(&deviceIp, data + 12, 4);

        if(deviceIp == g_littleTalks->myDevice->ip)
            break;

        device = LT_FoundDeviceByIp(deviceIp);
        if(device != NULL)
            device->localReceiveCounter = 0;

        contentDataSize = 0;
        memcpy(&contentDataSize, data + 16, 4);

        if(dataSize - LT_PACKET_HEADER_SIZE < contentDataSize)
            break;

        if(packetType == LTPacketType_Introduction)
        {
            LT_ProcessIntroduction(deviceIp, data + LT_PACKET_HEADER_SIZE, contentDataSize);
        }
        else if(packetType == LTPacketType_Public)
        {
            LT_ProcessChangeTopic(deviceIp, data + LT_PACKET_HEADER_SIZE, contentDataSize);
        }
        else if(packetType == LTPacketType_Confirmation)
        {
            if(device != NULL)
                LT_ProcessConfirmation(device, data + LT_PACKET_HEADER_SIZE, contentDataSize);
        }
        else if(packetType == LTPacketType_DisconnectDevice)
        {
            LT_ProcessDisconnectDevice(data + LT_PACKET_HEADER_SIZE, contentDataSize);
        }
        else if(packetType == LTPacketType_RequestForTopics)
        {
            device = LT_ProcessRequestTopics(&reqTopics, &reqTopicsCount, deviceIp, data + LT_PACKET_HEADER_SIZE, contentDataSize);
            if(reqTopicsCount != 0)
                LT_SendResponseTopics(device, reqTopics, reqTopicsCount);
            LT_FREE(reqTopics);
            reqTopics = NULL;
        }
        else if(packetType == LTPacketType_ResponseTopics)
        {
            LT_ProcessResponseTopics(deviceIp, data + LT_PACKET_HEADER_SIZE, contentDataSize);
        }
        else
        {
            LT_DetectError(LTError_UnknownLocalPacket, LT_CURR_CODE_LINE);
            break;
        }

        data += LT_PACKET_HEADER_SIZE + contentDataSize;
        dataSize -= LT_PACKET_HEADER_SIZE + contentDataSize;
    }
}
void LT_ProcessIntroduction(LT_UINT32 deviceIp, BYTE* data, int length)
{
    int i;
    int j;
    int deviceIndex;
    LT_DEVICE_FLAG devicesFlag;
    BYTE* dataIter;
    BYTE* dataEnd;
    int topicsCount;
    LT_DEVICE_ID deviceId;
    LT_TOPIC_ID topicId;
    LT_UINT32 counter;
    BOOL reconnect;

    struct LTTopic* foundTopic;
    struct LTDevice* device;
    BOOL foundNewTopic;
    BOOL isConnecting;

    dataIter = data;
    dataEnd = data + length;

    if(length < (int)sizeof(LT_DEVICE_ID) + 5)
    {
        LT_DetectError(LTError_IncorrectPacketSize, LT_CURR_CODE_LINE);
        return;
    }

    memcpy(&deviceId, dataIter, sizeof(LT_DEVICE_ID));
    dataIter += sizeof(LT_DEVICE_ID);
    memcpy(&reconnect, dataIter, 1);
    dataIter += 1;

    if(deviceId == g_littleTalks->myDevice->id)
        return;

    if(g_littleTalks->myDevice->id == deviceId)
        return;

    device = LT_FoundDevice(deviceId);

    if(reconnect && device != NULL)
    {
        if(device->isConnected)
        {
            if(device->autoRelease)
            {
                LTDevice_SetDisconnected(device);
                device = NULL;
            }
            else
                LTDevice_SetDisconnected(device);
        }
    }

    if(device == NULL)
    {
        if(g_littleTalks->devicesCount == LT_DEVICES_MAX_COUNT)
        {
            LT_DetectError(LTError_MaxDevicesCount, LT_CURR_CODE_LINE);
            return;
        }

        device = (struct LTDevice*)LT_MALLOC(sizeof(struct LTDevice));
        if(device == NULL)
        {
            LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);
            return;
        }

        LTDevice_Init(device, deviceId, deviceIp);

        LTDeviceList_Push(g_littleTalks->otherDeviceList, device);
        g_littleTalks->devicesCount++;
    }
    else
    {
        device->ip = deviceIp;
    }

    isConnecting = FALSE;
    if(!device->isLocalConnected)
    {
        device->localReceiveCounter = 0;
        device->isLocalConnected = TRUE;
    }

    device->receivedIntroduction = TRUE;
    if(!device->isConnected)
    {
        device->isConnected = TRUE;

        isConnecting = TRUE;
    }

    memcpy(&topicsCount, dataIter, 4);
    dataIter += 4;

    if(isConnecting)
        g_littleTalks->onConnected_funcPtr(device->id);

    foundNewTopic = FALSE;

    for(i = 0; i < topicsCount; i++)
    {
        if(dataEnd - dataIter < (int)sizeof(LT_TOPIC_ID))
            break;

        memcpy(&topicId, dataIter, sizeof(LT_TOPIC_ID));
        dataIter += sizeof(LT_TOPIC_ID);

        memcpy(&counter, dataIter, sizeof(LT_UINT32));
        dataIter += sizeof(LT_UINT32);

        foundTopic = LT_FoundTopic(topicId);

        if(foundTopic != NULL)
        {
            deviceIndex = LTDeviceList_IndexOf(g_littleTalks->otherDeviceList, device);
            devicesFlag = 1;
            for(j = 0; j < deviceIndex; j++)
                devicesFlag *= 2;

            if(!(foundTopic->devicesFlag & devicesFlag))
            {
                foundTopic->devicesFlag = foundTopic->devicesFlag | devicesFlag;
                foundNewTopic = TRUE;

                if(foundTopic->modifyCounter < counter)
                    foundTopic->modifyCounter = counter;

                g_littleTalks->onSubscribed_funcPtr(deviceId, topicId);
            }
        }
    }

    if(isConnecting && foundNewTopic)
        LT_SendIntroduction(deviceIp, FALSE);
}

void LT_ProcessChangeTopic(LT_UINT32 deviceIp, BYTE* data, int length)
{
    int j;
    BYTE* dataIter;
    BYTE* dataEnd;
    BOOL isConnecting;
    LT_DEVICE_ID deviceId;
    LT_TOPIC_ID topicId;
    LT_UINT32 counter;
    int deviceIndex;
    LT_DEVICE_FLAG devicesFlag;

    int valueSize;
    int dataSize;
    struct LTTopic* topic;
    struct LTDevice* device;
    BOOL isEqual;
    BYTE* valueIter;

    isConnecting = FALSE;

    if(length < (int)(8 + sizeof(LT_DEVICE_ID) + sizeof(LT_TOPIC_ID)))
    {
        LT_DetectError(LTError_IncorrectPacketSize, LT_CURR_CODE_LINE);
        return;
    }

    dataIter = data;
    dataEnd = data + length;

    dataSize = 0;
    memcpy(&dataSize, dataIter, 4);
    dataIter += 4;

    memcpy(&deviceId, dataIter, sizeof(LT_DEVICE_ID));
    dataIter += sizeof(LT_DEVICE_ID);

    if(deviceId == g_littleTalks->myDevice->id)
        return;

    memcpy(&topicId, dataIter, sizeof(LT_TOPIC_ID));
    dataIter += sizeof(LT_TOPIC_ID);

    memcpy(&counter, dataIter, 4);
    dataIter += 4;

    dataEnd = data + dataSize;

    topic = LT_FoundTopic(topicId);
    if(topic == NULL)
        return;

    device = LT_FoundDevice(deviceId);

    if(device == NULL)
    {
        if(g_littleTalks->devicesCount == LT_DEVICES_MAX_COUNT)
        {
            LT_DetectError(LTError_MaxDevicesCount, LT_CURR_CODE_LINE);
            return;
        }

        device = (struct LTDevice*)LT_MALLOC(sizeof(struct LTDevice));
        if(device == NULL)
        {
            LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);
            return;
        }

        LTDevice_Init(device, deviceId, deviceIp);

        LTDeviceList_Push(g_littleTalks->otherDeviceList, device);
        g_littleTalks->devicesCount++;
    }
    else
    {
        device->ip = deviceIp;
    }

    if(!device->isLocalConnected)
    {
        device->localReceiveCounter = 0;
        device->isLocalConnected = TRUE;
    }
    if(!device->isConnected)
    {
        isConnecting = TRUE;
        device->isConnected = TRUE;

        g_littleTalks->onConnected_funcPtr(device->id);
    }

    if(isConnecting)
        LT_SendIntroduction(deviceIp, FALSE);

    deviceIndex = LTDeviceList_IndexOf(g_littleTalks->otherDeviceList, device);
    devicesFlag = 1;
    for(j = 0; j < deviceIndex; j++)
        devicesFlag *= 2;

    if(counter == topic->modifyCounter)
    {
        if(g_littleTalks->myDevice->id < device->id)
        {
            if(!(topic->devicesFlag & devicesFlag))
            {
                topic->devicesFlag = topic->devicesFlag | devicesFlag;

                g_littleTalks->onSubscribed_funcPtr(deviceId, topicId);
            }

            LT_SendConfirmation(device, topicId, counter);
            return;
        }
    }
    else if(counter < topic->modifyCounter)
    {
        if(!(topic->devicesFlag & devicesFlag))
        {
            topic->devicesFlag = topic->devicesFlag | devicesFlag;

            g_littleTalks->onSubscribed_funcPtr(deviceId, topicId);
        }
        LT_SendConfirmation(device, topicId, topic->modifyCounter);
        return;
    }

    if(0x8fffffff < topic->modifyCounter)
        topic->modifyCounter = 0;

    topic->modifyCounter = counter;

    LT_SendConfirmation(device, topicId, topic->modifyCounter);

    valueSize = (dataEnd - dataIter);
    if(topic->valueSize < valueSize)
    {
        LT_DetectError(LTError_MaxValueSizeOverflow, LT_CURR_CODE_LINE);
        return;
    }

    valueIter = (BYTE*)(topic + 1);

    isEqual = TRUE;
    while(dataIter < dataEnd)
    {
        if(*valueIter != *dataIter)
        {
            isEqual = FALSE;
            break;
        }

        dataIter++;
        valueIter++;
    }

    if(isEqual)
    {
        if(!(topic->devicesFlag & devicesFlag))
        {
            topic->devicesFlag = topic->devicesFlag | devicesFlag;

            g_littleTalks->onSubscribed_funcPtr(deviceId, topicId);
        }
        return;
    }

    while(dataIter < dataEnd)
    {
        *valueIter = *dataIter;
        dataIter++;
        valueIter++;
    }

    if(!(topic->devicesFlag & devicesFlag))
    {
        topic->devicesFlag = topic->devicesFlag | devicesFlag;

        g_littleTalks->onSubscribed_funcPtr(deviceId, topicId);
    }

    g_littleTalks->onReceive_funcPtr(deviceId, topic->id, (BYTE*)(topic + 1), valueSize);
}
void LT_ProcessConfirmation(struct LTDevice* device, BYTE* data, int length)
{
    struct LTTopic* topic;
    struct LTDevice* deviceIter;
    LT_DEVICE_FLAG devFlagIter;
    LT_TOPIC_ID topicId;
    LT_UINT32 counter;

    if(length < (int)sizeof(LT_TOPIC_ID))
    {
        LT_DetectError(LTError_IncorrectPacketSize, LT_CURR_CODE_LINE);
        return;
    }

    memcpy(&topicId, data, sizeof(LT_TOPIC_ID));

    topic = LT_FoundTopic(topicId);

    if(topic == NULL)
        return;

    counter = 0;
    if((int)(sizeof(LT_TOPIC_ID) + sizeof(LT_UINT32)) <= length)
    {
        memcpy(&counter, data + sizeof(LT_TOPIC_ID), sizeof(LT_UINT32));
        if(topic->modifyCounter < counter)
            topic->modifyCounter = counter;
    }

    devFlagIter = 1;
    deviceIter = g_littleTalks->otherDeviceList;
    while(NULL != (deviceIter = LTDeviceList_Next(deviceIter)))
    {
        if(deviceIter == device)
            break;

        devFlagIter *= 2;
    }

    topic->publishDevicesFlag = topic->publishDevicesFlag & (~devFlagIter);
}

void LT_ProcessDisconnectDevice(BYTE* data, int length)
{
    LT_DEVICE_ID deviceId;
    struct LTDevice* device;

    if(length < (int)sizeof(LT_DEVICE_ID))
    {
        LT_DetectError(LTError_IncorrectPacketSize, LT_CURR_CODE_LINE);
        return;
    }

    memcpy(&deviceId, data, sizeof(LT_TOPIC_ID));

    device = LT_FoundDevice(deviceId);

    if(device == NULL)
        return;

    LTDevice_SetDisconnected(device);
}

void LT_SendRequestTopics(struct LTDevice* device)
{
    int dataSize;
    BYTE* data;

    if(g_littleTalks->isRunning && g_littleTalks->udpSocket == NULL)
        return;

    if(device->ip == 0)
        return;

    dataSize = LT_SizeRequestTopics(device);

    data = LT_CreateEmptyPacket(LTPacketType_RequestForTopics, &dataSize);

    if(data == NULL)
        return;

    LT_FillRequestTopics(data + LT_PACKET_HEADER_SIZE, device);

    LTPlatformAdapter_UDP_Send(g_littleTalks->udpSocket, device->ip, LT_UDP_PORT, data, LT_PACKET_HEADER_SIZE + dataSize);

    LT_FREE(data);
    data = NULL;
}
int LT_SizeRequestTopics(struct LTDevice* device)
{
    int dataSize;
    struct LTTopic* topicIter;
    LT_DEVICE_FLAG devFlag;

    devFlag = LTDevice_GetFlag(device->id);

    dataSize = sizeof(LT_DEVICE_ID) + 2;

    topicIter = g_littleTalks->topicsList;
    while(NULL != (topicIter = LTTopicList_Next(topicIter)))
    {
        if(!(topicIter->publishDevicesFlag & devFlag))
            continue;

        dataSize += sizeof(LT_TOPIC_ID) + 4;
    }

    return dataSize;
}
void LT_FillRequestTopics(BYTE* refData, struct LTDevice* device)
{
    BYTE* dataIter;
    LT_UINT16 topicsCount;
    struct LTTopic* topicIter;
    LT_DEVICE_FLAG devFlag;

    devFlag = LTDevice_GetFlag(device->id);

    dataIter = refData;

    memcpy(dataIter, &g_littleTalks->myDevice->id, sizeof(LT_DEVICE_ID));
    dataIter += sizeof(LT_DEVICE_ID);

    topicsCount = 0;
    topicIter = g_littleTalks->topicsList;
    while(NULL != (topicIter = LTTopicList_Next(topicIter)))
    {
        if(!(topicIter->publishDevicesFlag & devFlag))
            continue;

        topicsCount++;
    }

    memcpy(dataIter, &topicsCount, 2);
    dataIter += 2;

    topicIter = g_littleTalks->topicsList;
    while(NULL != (topicIter = LTTopicList_Next(topicIter)))
    {
        if(!(topicIter->publishDevicesFlag & devFlag))
            continue;

        memcpy(dataIter, &topicIter->id, sizeof(LT_TOPIC_ID));
        dataIter += sizeof(LT_TOPIC_ID);

        memcpy(dataIter, &topicIter->modifyCounter, 4);
        dataIter += 4;
    }
}
struct LTDevice* LT_ProcessRequestTopics(LT_TOPIC_ID** outTopics, LT_UINT16* outTopicsCount, LT_UINT32 deviceIp, BYTE* data, int length)
{
    BYTE* dataIter;
    LT_DEVICE_ID deviceId;
    LT_TOPIC_ID topicId;
    LT_UINT32 counter;
    LT_UINT16 j;
    LT_UINT16 topicsCount;
    LT_UINT16 respTopicsCount;
    LT_UINT16 deviceIndex;
    LT_DEVICE_FLAG devicesFlag;
    struct LTTopic* topic;
    struct LTDevice* device;

    *outTopics = NULL;
    *outTopicsCount = 0;
    device = NULL;

    if(length < (int)(sizeof(LT_DEVICE_ID) + 2))
    {
        LT_DetectError(LTError_IncorrectPacketSize, LT_CURR_CODE_LINE);
        return NULL;
    }

    dataIter = data;

    memcpy(&deviceId, dataIter, sizeof(LT_DEVICE_ID));
    dataIter += sizeof(LT_DEVICE_ID);

    if(deviceId == g_littleTalks->myDevice->id)
        return NULL;

    if(deviceIp != 0)
        device = LTDevice_SetConnected(deviceId);

    if(device == NULL)
        return NULL;

    if(deviceIp != 0)
        device->ip = deviceIp;

    memcpy(&topicsCount, dataIter, 2);
    dataIter += 2;

    *outTopics = (LT_TOPIC_ID*)LT_MALLOC(topicsCount * sizeof(LT_TOPIC_ID));

    if(*outTopics == NULL)
    {
        LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);
        return NULL;
    }

    deviceIndex = (LT_UINT16)LTDeviceList_IndexOf(g_littleTalks->otherDeviceList, device);
    devicesFlag = 1;
    for(j = 0; j < deviceIndex; j++)
        devicesFlag *= 2;

    respTopicsCount = 0;
    for(j = 0; j < topicsCount; j++)
    {
        memcpy(&topicId, dataIter, sizeof(LT_TOPIC_ID));
        dataIter += sizeof(LT_TOPIC_ID);

        (*outTopics)[j] = topicId;

        memcpy(&counter, dataIter, 4);
        dataIter += 4;

        topic = LT_FoundTopic(topicId);
        if(topic == NULL)
            continue;

        respTopicsCount++;

        if(!(topic->devicesFlag & devicesFlag))
        {
            topic->devicesFlag = topic->devicesFlag | devicesFlag;

            if(topic->modifyCounter < counter)
                topic->modifyCounter = counter;

            g_littleTalks->onSubscribed_funcPtr(deviceId, topicId);
        }
    }

    *outTopicsCount = respTopicsCount;

    return device;
}

void LT_SendResponseTopics(struct LTDevice* device, LT_TOPIC_ID* topics, LT_UINT16 topicsCount)
{
    int dataSize;
    BYTE* data;

    if(device->ip == 0)
        return;

    dataSize = LT_SizeResponseTopics(topics, topicsCount);

    data = LT_CreateEmptyPacket(LTPacketType_ResponseTopics, &dataSize);

    if(data == NULL)
        return;

    LT_FillResponseTopics(data + LT_PACKET_HEADER_SIZE, topics, topicsCount);

    LTPlatformAdapter_UDP_Send(g_littleTalks->udpSocket, device->ip, LT_UDP_PORT, data, LT_PACKET_HEADER_SIZE + dataSize);

    LT_FREE(data);
    data = NULL;
}
int LT_SizeResponseTopics(LT_TOPIC_ID* topics, LT_UINT16 topicsCount)
{
    LT_UINT16 j;
    int dataSize;
    struct LTTopic* topic;

    dataSize = sizeof(LT_DEVICE_ID) + 2;

    for(j = 0; j < topicsCount; j++)
    {
        topic = LT_FoundTopic(topics[j]);
        if(topic == NULL)
            continue;

        dataSize += sizeof(LT_TOPIC_ID) + 4 + 2 + topic->valueSize;
    }

    return dataSize;
}
void LT_FillResponseTopics(BYTE* refData, LT_TOPIC_ID* topics, LT_UINT16 topicsCount)
{
    LT_UINT16 j;
    struct LTTopic* topic;
    BYTE* dataIter;

    dataIter = refData;

    memcpy(dataIter, &g_littleTalks->myDevice->id, sizeof(LT_DEVICE_ID));
    dataIter += sizeof(LT_DEVICE_ID);

    memcpy(dataIter, &topicsCount, 2);
    dataIter += 2;

    for(j = 0; j < topicsCount; j++)
    {
        topic = LT_FoundTopic(topics[j]);
        if(topic == NULL)
            continue;

        memcpy(dataIter, &topic->id, sizeof(LT_TOPIC_ID));
        dataIter += sizeof(LT_TOPIC_ID);

        memcpy(dataIter, &topic->modifyCounter, 4);
        dataIter += 4;

        memcpy(dataIter, &topic->valueSize, 2);
        dataIter += 2;

        memcpy(dataIter, topic + 1, topic->valueSize);
        dataIter += topic->valueSize;
    }
}
struct LTDevice* LT_ProcessResponseTopics(LT_UINT32 deviceIp, BYTE* data, int length)
{
    BYTE* dataIter;
    LT_UINT16 topicsCount;
    LT_UINT16 valueSize;
    LT_UINT16 j;
    LT_UINT32 counter;
    LT_DEVICE_ID deviceId;
    LT_TOPIC_ID topicId;
    LT_UINT16 deviceIndex;
    LT_DEVICE_FLAG devicesFlag;

    struct LTDevice* device;
    struct LTTopic* topic;

    device = NULL;
    topic = NULL;

    if(length < (int)(sizeof(LT_DEVICE_ID) + 2))
    {
        LT_DetectError(LTError_IncorrectPacketSize, LT_CURR_CODE_LINE);
        return NULL;
    }

    dataIter = data;

    memcpy(&deviceId, dataIter, sizeof(LT_DEVICE_ID));
    dataIter += sizeof(LT_DEVICE_ID);

    if(deviceId == g_littleTalks->myDevice->id)
        return NULL;

    if(deviceIp != 0)
        device = LTDevice_SetConnected(deviceId);

    if(device == NULL)
        return NULL;

    if(deviceIp != 0)
        device->ip = deviceIp;

    deviceIndex = (LT_UINT16)LTDeviceList_IndexOf(g_littleTalks->otherDeviceList, device);
    devicesFlag = 1;
    for(j = 0; j < deviceIndex; j++)
        devicesFlag *= 2;

    device->waitingForResponseCounter = 0;
    device->waitingForResponse = FALSE;

    memcpy(&topicsCount, dataIter, 2);
    dataIter += 2;

    for(j = 0; j < topicsCount; j++)
    {
        memcpy(&topicId, dataIter, sizeof(LT_TOPIC_ID));
        dataIter += sizeof(LT_TOPIC_ID);

        memcpy(&counter, dataIter, 4);
        dataIter += 4;

        memcpy(&valueSize, dataIter, 2);
        dataIter += 2;

        topic = LT_FoundTopic(topicId);
        if(topic == NULL)
        {
            dataIter += valueSize;
            continue;
        }

        if(valueSize != topic->valueSize)
        {
            dataIter += valueSize;
            continue;
        }

        if(topic->modifyCounter <= counter)
        {
            topic->isChanged = FALSE;

            memcpy(topic + 1, dataIter, valueSize);
            topic->modifyCounter = counter;
        }

        dataIter += valueSize;

        if(!(topic->devicesFlag & devicesFlag))
        {
            topic->devicesFlag = topic->devicesFlag | devicesFlag;

            g_littleTalks->onSubscribed_funcPtr(deviceId, topicId);
        }
        else
        {
            topic->publishDevicesFlag = topic->publishDevicesFlag & (~devicesFlag);
        }

        g_littleTalks->onReceive_funcPtr(deviceId, topic->id, (BYTE*)(topic + 1), valueSize);
    }

    return device;
}

void LT_SendIntroductionBroadcast()
{
    LT_UINT32 bcastIp;
    bcastIp = LTPlatformAdapter_GetGateway() | ((~LTPlatformAdapter_GetNetworkMask()) & 0xffffffff);

    LT_SendIntroduction(bcastIp, FALSE);
}

void LT_SendIntroduction(LT_UINT32 ip, BOOL reconnect)
{
    int dataSize;
    int otherTopicCount;
    BYTE* data;
    BYTE* dataIter;
    struct LTTopic* topicIter;
    int blocksCount;
    int topicsCount;
    int sendTopicsCount;
    int fromTopicsIndex;
    int maxDataSize;
    int topicIndex;

    topicsCount = LTTopicList_Size(g_littleTalks->topicsList);

    maxDataSize = LT_UDP_PACKET_SIZE_MAX - LT_PACKET_HEADER_SIZE - (sizeof(LT_DEVICE_ID) + sizeof(BOOL) + 4);
    sendTopicsCount = maxDataSize / (sizeof(LT_TOPIC_ID) + sizeof(LT_UINT32) + 1);
    maxDataSize = sendTopicsCount * (sizeof(LT_TOPIC_ID) + sizeof(LT_UINT32) + 1);
    blocksCount = (topicsCount * (sizeof(LT_TOPIC_ID) + sizeof(LT_UINT32) + 1) + maxDataSize - 1) / maxDataSize;
    fromTopicsIndex = (g_littleTalks->sendIntroductionCounter % blocksCount) * sendTopicsCount;
    dataSize = sizeof(LT_DEVICE_ID) + sizeof(BOOL) + 4 + sendTopicsCount * (sizeof(LT_TOPIC_ID) + sizeof(LT_UINT32) + 1);

    data = LT_CreateEmptyPacket(LTPacketType_Introduction, &dataSize);
    if(data == NULL)
        return;

    dataIter = data + LT_PACKET_HEADER_SIZE;

    memcpy(dataIter, &g_littleTalks->myDevice->id, sizeof(LT_DEVICE_ID));
    dataIter += sizeof(LT_DEVICE_ID);

    memcpy(dataIter, &reconnect, 1);
    dataIter += 1;

    otherTopicCount = topicsCount;
    memcpy(dataIter, &otherTopicCount, 4);
    dataIter += 4;

    topicIter = g_littleTalks->topicsList;

    topicIndex = 0;
    while(topicIndex < fromTopicsIndex)
    {
        topicIter = LTTopicList_Next(topicIter);
        topicIndex++;
    }

    while(topicIndex < fromTopicsIndex + sendTopicsCount)
    {
        topicIter = LTTopicList_Next(topicIter);
        if(topicIter == NULL)
            break;

        memcpy(dataIter, &topicIter->id, sizeof(LT_TOPIC_ID));
        dataIter += sizeof(LT_TOPIC_ID);

        memcpy(dataIter, &topicIter->modifyCounter, sizeof(LT_UINT32));
        dataIter += sizeof(LT_UINT32);

        topicIndex++;
    }

    g_littleTalks->sendIntroductionCounter++;

    if(ip != 0)
        LTPlatformAdapter_UDP_Send(g_littleTalks->udpSocket, ip, LT_UDP_PORT, data, LT_PACKET_HEADER_SIZE + dataSize);

    LT_FREE(data);
    data = NULL;
}
void LT_SendDisconnect(LT_UINT32 ip)
{
    int dataSize;
    BYTE* data;

    dataSize = sizeof(LT_DEVICE_ID);
    data = LT_CreateEmptyPacket(LTPacketType_DisconnectDevice, &dataSize);
    if(data == NULL)
        return;

    memcpy(data + LT_PACKET_HEADER_SIZE, &g_littleTalks->myDevice->id, sizeof(LT_DEVICE_ID));

    if(ip != 0)
        LTPlatformAdapter_UDP_Send(g_littleTalks->udpSocket, ip, LT_UDP_PORT, data, LT_PACKET_HEADER_SIZE + dataSize);

    LT_FREE(data);
}
void LT_SendConfirmation(struct LTDevice* device, LT_TOPIC_ID topicId, LT_UINT32 topicCounter)
{
    int dataSize;
    BYTE* data;

    if(device->ip == 0)
        return;

    dataSize = sizeof(LT_TOPIC_ID) + sizeof(LT_UINT32);
    data = LT_CreateEmptyPacket(LTPacketType_Confirmation, &dataSize);
    if(data == NULL)
        return;

    memcpy(data + LT_PACKET_HEADER_SIZE, &topicId, sizeof(LT_TOPIC_ID));
    memcpy(data + LT_PACKET_HEADER_SIZE + sizeof(LT_TOPIC_ID), &topicCounter, sizeof(LT_UINT32));

    LTPlatformAdapter_UDP_Send(g_littleTalks->udpSocket, device->ip, LT_UDP_PORT, data, LT_PACKET_HEADER_SIZE + dataSize);

    LT_FREE(data);
}

void LT_SendTopic(struct LTTopic* topic, LT_DEVICE_FLAG devicesFlag, BYTE* value, LT_UINT16 valueSize)
{
    int dataSize;
    int totalDataSize;
    BYTE* data;
    BYTE* dataIter;
    struct LTDevice* deviceIter;
    LT_DEVICE_FLAG deviceFlagIter;

    if(g_littleTalks == NULL)
        return;

    if(g_littleTalks->isRunning && g_littleTalks->udpSocket == NULL)
        return;

    if(topic->valueSize < valueSize)
    {
        LT_DetectError(LTError_MaxValueSizeOverflow, LT_CURR_CODE_LINE);
        return;
    }

    memcpy(topic + 1, value, valueSize);

    dataSize = 4 + sizeof(LT_DEVICE_ID) + sizeof(LT_TOPIC_ID) + 4 + valueSize;

    totalDataSize = dataSize;
    data = LT_CreateEmptyPacket(LTPacketType_Public, &totalDataSize);

    if(data == NULL)
    {
        deviceFlagIter = 1;
        deviceIter = g_littleTalks->otherDeviceList;
        while(NULL != (deviceIter = LTDeviceList_Next(deviceIter)))
        {
            if(devicesFlag & deviceFlagIter)
                topic->publishDevicesFlag |= deviceFlagIter;

            deviceFlagIter *= 2;
        }
        return;
    }

    totalDataSize += LT_PACKET_HEADER_SIZE;

    dataIter = data + LT_PACKET_HEADER_SIZE;

    memcpy(dataIter, &dataSize, 4);
    dataIter += 4;

    memcpy(dataIter, &g_littleTalks->myDevice->id, sizeof(LT_DEVICE_ID));
    dataIter += sizeof(LT_DEVICE_ID);

    memcpy(dataIter, &topic->id, sizeof(LT_TOPIC_ID));
    dataIter += sizeof(LT_TOPIC_ID);

    memcpy(dataIter, &topic->modifyCounter, 4);
    dataIter += 4;

    memcpy(dataIter, topic + 1, valueSize);
    dataIter += valueSize;

    deviceFlagIter = 1;
    deviceIter = g_littleTalks->otherDeviceList;
    while(NULL != (deviceIter = LTDeviceList_Next(deviceIter)))
    {
        if(devicesFlag & deviceFlagIter)
        {
            if(deviceIter->isConnected)
            {
                if(deviceIter->ip != 0)
                    LTPlatformAdapter_UDP_Send(g_littleTalks->udpSocket, deviceIter->ip, LT_UDP_PORT, data, totalDataSize);
            }
            topic->publishDevicesFlag |= deviceFlagIter;
        }
        deviceFlagIter *= 2;
    }

    topic->publishDevicesFlag = devicesFlag;
    topic->publishedTick = g_littleTalks->tick;

    LT_FREE(data);
    data = NULL;
}

struct LTTopic* LT_FoundTopic(LT_TOPIC_ID topicId)
{
    struct LTTopic* topicIter;

    topicIter = g_littleTalks->topicsList;
    while(NULL != (topicIter = LTTopicList_Next(topicIter)))
        if(topicIter->id == topicId)
            return topicIter;

    return NULL;
}
struct LTDevice* LT_FoundDevice(LT_DEVICE_ID deviceId)
{
    struct LTDevice* deviceIter;

    if(g_littleTalks == NULL)
        return NULL;

    deviceIter = g_littleTalks->otherDeviceList;
    while(NULL != (deviceIter = LTDeviceList_Next(deviceIter)))
        if(deviceIter->id == deviceId)
            return deviceIter;

    return NULL;
}
struct LTDevice* LT_FoundDeviceByIp(LT_UINT32 ip)
{
    struct LTDevice* deviceIter;

    deviceIter = g_littleTalks->otherDeviceList;
    while(NULL != (deviceIter = LTDeviceList_Next(deviceIter)))
        if(deviceIter->ip == ip)
            return deviceIter;

    return NULL;
}

BYTE* LT_CreateEmptyPacket(enum LTPacketType packetType, int* refDataSize)
{
    int dataSize;
    int newDataSize;
    BYTE* dataIter;
    BYTE* data;
    BYTE* end;

    dataSize = *refDataSize;
    newDataSize = dataSize;

    data = LT_MALLOC(LT_PACKET_HEADER_SIZE + newDataSize);
    if(data == NULL)
    {
        LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);
        return NULL;
    }

    dataIter = data;
    *dataIter = 'L'; dataIter++;
    *dataIter = 'T'; dataIter++;
    *dataIter = 'L'; dataIter++;
    *dataIter = 'T'; dataIter++;

    *dataIter = g_littleTalks->majorVersion; dataIter++;
    *dataIter = g_littleTalks->minorVersion; dataIter++;
    *dataIter = (BYTE)packetType; dataIter++;

    ///Reserved for future
    *dataIter = 0; dataIter++;
    *dataIter = 0; dataIter++;
    *dataIter = 0; dataIter++;
    *dataIter = 0; dataIter++;
    *dataIter = 0; dataIter++;

    memcpy(dataIter, &g_littleTalks->myDevice->ip, 4);
    dataIter += 4;

    memcpy(dataIter, &newDataSize, 4);
    dataIter += 4;

    end = dataIter + newDataSize;
    dataIter += dataSize;

    while(dataIter != end)
    {
        *dataIter = (char)(rand() % 255);
        dataIter++;
    }

    *refDataSize = newDataSize;

    return data;
}
BOOL LT_CheckPacket(BYTE* data, int dataSize)
{
    unsigned char majorVersion;
    unsigned char minorVersion;

    if(dataSize < LT_PACKET_HEADER_SIZE)
        return FALSE;

    if(data[0] != 'L' || data[1] != 'T' || data[2] != 'L' || data[3] != 'T')
        return FALSE;

    majorVersion = data[4];
    minorVersion = data[5];

    if(g_littleTalks->majorVersion != majorVersion || g_littleTalks->minorVersion != minorVersion)
    {
        LT_DetectError(LTError_IncompatibleLTVersion, LT_CURR_CODE_LINE);
        return FALSE;
    }

    return TRUE;
}

void LT_DetectError(enum LTError error, int currCodeLine)
{
    printf("LTError %d : %d", error, currCodeLine);fflush(stdout);
}



