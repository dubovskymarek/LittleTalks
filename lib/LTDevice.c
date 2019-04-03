#include "LTDevice.h"

#include "LittleTalksPrivate.h"

extern struct LittleTalks* g_littleTalks;

BOOL LTDeviceList_IsEmpty(struct LTDevice* head)
{
    return head->nextNode == NULL;
}
int LTDeviceList_Size(struct LTDevice* head)
{
    int size;
    struct LTDevice* node;

    size = 0;
    node = head->nextNode;
    while(node != 0)
    {
        node = node->nextNode;
        size++;
    }

    return size;
}
struct LTDevice* LTDeviceList_Next(struct LTDevice* node)
{
    return node != NULL ? node->nextNode : NULL;
}
int LTDeviceList_IndexOf(struct LTDevice* head, struct LTDevice* node)
{
    int index;
    struct LTDevice* nodeIter;

    index = 0;
    nodeIter = head->nextNode;
    while(nodeIter != 0)
    {
        if(nodeIter == node)
            return index;
        nodeIter = nodeIter->nextNode;
        index++;
    }

    return -1;
}
void LTDeviceList_Push(struct LTDevice* head, struct LTDevice* newNode)
{
    struct LTDevice* node;

    node = head;
    while(node->nextNode != NULL)
        node = node->nextNode;

    node->nextNode = newNode;
    newNode->nextNode = NULL;
}

void LTDeviceList_Clear(struct LTDevice* head)
{
    struct LTDevice* node;
    struct LTDevice* nextNode;

    node = head;
    while(node->nextNode != NULL)
    {
        nextNode = node->nextNode->nextNode;

        LTDevice_Uninit(node->nextNode);
        LT_FREE(node->nextNode);

        node->nextNode = nextNode;
    }
}


void LTDevice_Init(struct LTDevice* device, const LT_DEVICE_ID id, const LT_UINT32 ip)
{
    memset(device, 0, sizeof(struct LTDevice));

    device->id = id;
    device->ip = ip;

    device->isConnected = FALSE;
    device->isLocalConnected = FALSE;
    device->autoRelease = TRUE;
    device->remoteSubscribed = 0;
    device->receivedIntroduction = FALSE;
    device->toExpectLocal = FALSE;
    device->waitingForResponse = FALSE;
    device->waitingForResponseCounter = 0;
    device->receivedIntroductionCounter = 0;

    device->lastTick = 0;
    device->localReceiveCounter = 0;

    device->nextNode = NULL;
}
void LTDevice_Uninit(struct LTDevice* device)
{
    device->id = 0;
    device->ip = 0;

    device->isConnected = FALSE;
    device->isLocalConnected = FALSE;
    device->autoRelease = TRUE;
    device->remoteSubscribed = 0;
    device->receivedIntroduction = FALSE;
    device->toExpectLocal = FALSE;
    device->waitingForResponse = FALSE;
    device->waitingForResponseCounter = 0;
    device->receivedIntroductionCounter = 0;

    device->lastTick = 0;
    device->localReceiveCounter = 0;

    device->nextNode = NULL;
}
LT_DEVICE_FLAG LTDevice_GetFlag(LT_DEVICE_ID deviceId)
{
    struct LTDevice* deviceIter;
    LT_DEVICE_FLAG deviceFlagIter;

    deviceFlagIter = 1;
    deviceIter = g_littleTalks->otherDeviceList;
    while(NULL != (deviceIter = LTDeviceList_Next(deviceIter)))
    {
        if(deviceIter->id == deviceId)
            return deviceFlagIter;

        deviceFlagIter *= 2;
    }

    return 0;
}
struct LTDevice* LTDevice_SetConnected(LT_DEVICE_ID deviceId)
{
    struct LTDevice* device;

    device = LT_FoundDevice(deviceId);

    if(device == NULL)
    {
        if(g_littleTalks->devicesCount == LT_DEVICES_MAX_COUNT)
        {
            LT_DetectError(LTError_MaxDevicesCount, LT_CURR_CODE_LINE);
            return NULL;
        }

        device = (struct LTDevice*)LT_MALLOC(sizeof(struct LTDevice));
        if(device == NULL)
        {
            LT_DetectError(LTError_NoAvailableMemory, LT_CURR_CODE_LINE);
            return NULL;
        }

        LTDevice_Init(device, deviceId, 0);
        LTDeviceList_Push(g_littleTalks->otherDeviceList, device);
        g_littleTalks->devicesCount++;
    }

    if(device->isConnected)
        return device;

    device->localReceiveCounter = 0;
    device->isLocalConnected = TRUE;

    device->isConnected = TRUE;
    g_littleTalks->onConnected_funcPtr(deviceId);

    return device;
}
struct LTDevice* LTDevice_SetDisconnected(struct LTDevice* device)
{
    LT_DEVICE_FLAG devFlag;
    struct LTTopic* topicIter;
    struct LTDevice* deviceIter;

    if(!device->isConnected)
        return device;

    device->isConnected = FALSE;
    device->isLocalConnected = FALSE;
    device->remoteSubscribed = 0;
    device->receivedIntroduction = FALSE;
    device->waitingForResponse = FALSE;
    device->localReceiveCounter = 0;
    device->receivedIntroductionCounter = 0;

    g_littleTalks->onDisconnected_funcPtr(device->id);

    if(device->autoRelease)
    {
        device = LTDeviceList_Erase(g_littleTalks->otherDeviceList, device, g_littleTalks->topicsList);
        g_littleTalks->devicesCount--;
    }
    else
    {
        devFlag = 1;
        deviceIter = g_littleTalks->otherDeviceList;
        while(NULL != (deviceIter = LTDeviceList_Next(deviceIter)))
        {
            if(deviceIter == device)
                break;

            devFlag *= 2;
        }

        topicIter = g_littleTalks->topicsList;
        while(NULL != (topicIter = LTTopicList_Next(topicIter)))
        {
            topicIter->devicesFlag = topicIter->devicesFlag & (~devFlag);
            topicIter->publishDevicesFlag = topicIter->publishDevicesFlag & (~devFlag);
        }
    }

    return device;
}
void LTDevice_SubscribeTopic(struct LTDevice* device, struct LTTopic* topic)
{
    int j;
    int deviceIndex;
    LT_DEVICE_FLAG devicesFlag;

    deviceIndex = LTDeviceList_IndexOf(g_littleTalks->otherDeviceList, device);
    devicesFlag = 1;
    for(j = 0; j < deviceIndex; j++)
        devicesFlag *= 2;

    if(!(topic->devicesFlag & devicesFlag))
    {
        topic->devicesFlag = topic->devicesFlag | devicesFlag;

        g_littleTalks->onSubscribed_funcPtr(device->id, topic->id);
    }
}

struct LTDevice* LTDeviceList_Erase(struct LTDevice* deviceHead, struct LTDevice* oldNode, struct LTTopic* topicHead)
{
    struct LTDevice* deviceIter;
    struct LTDevice* prevDeviceIter;
    struct LTTopic* topicIter;
    LT_UINT32 deviceIndex;
    LT_DEVICE_FLAG devFlag;
    LT_DEVICE_FLAG prefixMask;
    LT_DEVICE_FLAG sufixMask;
    LT_UINT32 i;

    deviceIndex = LTDeviceList_IndexOf(deviceHead, oldNode);

    deviceIter = deviceHead->nextNode;
    prevDeviceIter = deviceHead;
    while(deviceIter != NULL)
    {
        if(deviceIter == oldNode)
            break;
        prevDeviceIter = deviceIter;
        deviceIter = deviceIter->nextNode;
    }

    if(deviceIter == NULL)
        return NULL;

    prefixMask = 0;
    devFlag = 1;
    i = 0;
    while(i < deviceIndex)
    {
        prefixMask |= devFlag;
        devFlag *= 2;
        i++;
    }

    devFlag *= 2;
    i++;

    sufixMask = 0;
    while(i < LT_DEVICES_MAX_COUNT)
    {
        sufixMask |= devFlag;
        devFlag *= 2;
        i++;
    }

    topicIter = topicHead;
    while(NULL != (topicIter = LTTopicList_Next(topicIter)))
    {
        devFlag = topicIter->devicesFlag;
        topicIter->devicesFlag = (devFlag & prefixMask) | ((devFlag & sufixMask) / 2);

        devFlag = topicIter->publishDevicesFlag;
        topicIter->publishDevicesFlag = (devFlag & prefixMask) | ((devFlag & sufixMask) / 2);
    }

    prevDeviceIter->nextNode = deviceIter->nextNode;
    LTDevice_Uninit(deviceIter);

    LT_FREE(deviceIter);

    return prevDeviceIter;
}

