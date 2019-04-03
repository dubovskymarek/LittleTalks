#ifndef LTTOPIC_H
#define LTTOPIC_H

#include "LTPlatformAdapter.h"
#include "LittleTalksSettings.h"

#define LT_DISABLE_REMOTE 0xffff

struct LTTopic
{
    LT_TOPIC_ID id;
    LT_UINT16 valueSize;

    LT_DEVICE_FLAG devicesFlag;
    LT_DEVICE_FLAG publishDevicesFlag;
    LT_UINT32 modifyCounter;
    LT_UINT32 publishedTick;

    BOOL isChanged;
    BOOL isReceivedFromServer;
    BOOL remoteSubscribed;

    struct LTTopic* nextNode;
};


BOOL LTTopicList_IsEmpty(struct LTTopic* head);
int LTTopicList_Size(struct LTTopic* head);
struct LTTopic* LTTopicList_Next(struct LTTopic* node);
int LTTopicList_IndexOf(struct LTTopic* head, struct LTTopic* node);
void LTTopicList_Push(struct LTTopic* head, struct LTTopic* newNode);
void LTTopicList_Clear(struct LTTopic* head);

void LTTopic_Init(struct LTTopic* topic, const LT_TOPIC_ID id, const LT_UINT16 valueSize);
void LTTopic_Uninit(struct LTTopic* topic);
BOOL LTTopic_IsValueEqual(struct LTTopic* topic, BYTE* newValue, int newValueSize);
BOOL LTTopic_CheckModifyCounter(struct LTTopic* topic, LT_DEVICE_ID sourceDeviceId, LT_UINT32 modifyCounter);

#endif // LTTOPIC_H
