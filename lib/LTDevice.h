#ifndef LTDEVICE_H
#define LTDEVICE_H

#include "LTPlatformAdapter.h"
#include "LittleTalksSettings.h"

struct LTDevice
{
    LT_DEVICE_ID id;
    LT_UINT32 ip;
    BOOL isConnected;
    BOOL isLocalConnected;
    BOOL autoRelease;
    LT_UINT8 remoteSubscribed;
    BOOL receivedIntroduction;
    BOOL toExpectLocal;
    BOOL waitingForResponse;
    LT_UINT8 waitingForResponseCounter;
    LT_UINT8 receivedIntroductionCounter;

    LT_UINT8 lastTick;
    LT_UINT16 localReceiveCounter;

    struct LTDevice* nextNode;
};

struct LTTopic;

BOOL LTDeviceList_IsEmpty(struct LTDevice* head);
int LTDeviceList_Size(struct LTDevice* head);
struct LTDevice* LTDeviceList_Next(struct LTDevice* node);
int LTDeviceList_IndexOf(struct LTDevice* head, struct LTDevice* node);
struct LTDevice* LTDeviceList_Erase(struct LTDevice* deviceHead, struct LTDevice* oldNode, struct LTTopic* topicHead);
void LTDeviceList_Push(struct LTDevice* head, struct LTDevice* newNode);
void LTDeviceList_Clear(struct LTDevice* head);

void LTDevice_Init(struct LTDevice* device, const LT_DEVICE_ID id, const LT_UINT32 ip);
void LTDevice_Uninit(struct LTDevice* device);
LT_DEVICE_FLAG LTDevice_GetFlag(LT_DEVICE_ID deviceId);
struct LTDevice* LTDevice_SetConnected(LT_DEVICE_ID deviceId);
struct LTDevice* LTDevice_SetDisconnected(struct LTDevice* device);

#endif // LTDEVICE_H
