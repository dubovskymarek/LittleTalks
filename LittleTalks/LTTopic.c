#include "LTTopic.h"

#include "LittleTalksPrivate.h"

extern struct LittleTalks* g_littleTalks;

void LTTopic_Init(struct LTTopic* topic, const LT_TOPIC_ID id, const LT_UINT16 valueSize)
{
    memset(topic, 0, sizeof(struct LTTopic) + valueSize);

    topic->id = id;
    topic->valueSize = valueSize;

    topic->devicesFlag = 0;
    topic->publishDevicesFlag = 0;
    topic->isReceivedFromServer = FALSE;
    topic->isChanged = FALSE;
    topic->modifyCounter = 0;
    topic->publishedTick = 0;
    topic->remoteSubscribed = FALSE;

    topic->nextNode = NULL;
}
void LTTopic_Uninit(struct LTTopic* topic)
{
    memset(topic, 0, sizeof(struct LTTopic) + topic->valueSize);
}


BOOL LTTopicList_IsEmpty(struct LTTopic* head)
{
    return head->nextNode == NULL;
}
int LTTopicList_Size(struct LTTopic* head)
{
    int size;
    struct LTTopic* node;

    size = 0;
    node = head->nextNode;
    while(node != 0)
    {
        node = node->nextNode;
        size++;
    }

    return size;
}
struct LTTopic* LTTopicList_Next(struct LTTopic* node)
{
    return node != NULL ? node->nextNode : NULL;
}
int LTTopicList_IndexOf(struct LTTopic* head, struct LTTopic* node)
{
    int index;
    struct LTTopic* nodeIter;

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
void LTTopicList_Push(struct LTTopic* head, struct LTTopic* newNode)
{
    struct LTTopic* node;

    node = head;
    while(node->nextNode != NULL)
        node = node->nextNode;

    node->nextNode = newNode;
    newNode->nextNode = NULL;
}
void LTTopicList_Clear(struct LTTopic* head)
{
    struct LTTopic* node;
    struct LTTopic* nextNode;

    node = head;
    while(node->nextNode != NULL)
    {
        nextNode = node->nextNode->nextNode;

        LTTopic_Uninit(node->nextNode);
        LT_FREE(node->nextNode);

        node->nextNode = nextNode;
    }
}
BOOL LTTopic_CheckModifyCounter(struct LTTopic* topic, LT_DEVICE_ID sourceDeviceId, LT_UINT32 modifyCounter)
{
    BOOL isNew;

    isNew = TRUE;

    if(modifyCounter == topic->modifyCounter)
    {
        if(g_littleTalks->myDevice->id < sourceDeviceId)
            isNew = FALSE;
    }
    else if(modifyCounter < topic->modifyCounter)
        isNew = FALSE;

    if(isNew)
        topic->modifyCounter = modifyCounter;

    if(0x8fffffff < topic->modifyCounter)
        topic->modifyCounter = 0;

    return isNew;
}

BOOL LTTopic_IsValueEqual(struct LTTopic* topic, BYTE* newValue, int newValueSize)
{
    BYTE* valueIter;
    BYTE* valueEnd;
    BYTE* receivedValueIter;

    if(topic->valueSize < newValueSize)
        return FALSE;

    valueIter = (BYTE*)(topic + 1);
    valueEnd = valueIter + newValueSize;
    receivedValueIter = newValue;

    while(valueIter < valueEnd)
    {
        if(*valueIter != *receivedValueIter)
            return FALSE;

        receivedValueIter++;
        valueIter++;
    }

    return TRUE;
}

