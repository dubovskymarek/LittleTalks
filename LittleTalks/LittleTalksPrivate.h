#ifndef LITTLETALKSPRIVATE_H
#define LITTLETALKSPRIVATE_H

#include "LTTopic.h"
#include "LTDevice.h"
#include "LTPlatformAdapter.h"
#include "LittleTalksSettings.h"

struct LTDevice;
struct LittleTalks;

enum LTPacketType
{
    LTPacketType_None = 0,
    LTPacketType_Introduction = 1,
    LTPacketType_Public = 2,
    LTPacketType_Confirmation = 3,
    LTPacketType_DisconnectDevice = 4,
    LTPacketType_RequestForTopics = 5,
    LTPacketType_ResponseTopics = 6,
};

///
/// \enum The LTError
///
/// \brief Error types
///
enum LTError
{
    LTError_NoError = 0, /**< No error */
    LTError_IncompatibleLTVersion = 1, /**< Incompatible LittleTalks version */
    LTError_NoAvailableMemory = 2, /**< No available memory */
    LTError_MaxSizeOfUdpPacketOverflow = 3, /**< Maximum size of udp packet (1400) is overflow */
    LTError_MaxValueSizeOverflow = 4, /**< Maximum number of bytes value is overflow */
    LTError_MaxDevicesCount = 5, /**< Overflow number of maximum devices */
    LTError_DeviceWithoutIPAddress = 6, /**< Device has not assigned IP address */
    LTError_IncorrectPacketSize = 8, /**< Incorrect size of incoming packet */
    LTError_UnknownLocalPacket = 9, /**< Unknown incoming udp packet in local network */
    LTError_UnknownError = 255, /**< Unknown error */
};

typedef void (*LT_OnConnect_Func)(LT_DEVICE_ID deviceId);
typedef void (*LT_OnDisconnect_Func)(LT_DEVICE_ID deviceId);
typedef void (*LT_OnSubscribed_Func)(LT_DEVICE_ID deviceId, LT_TOPIC_ID topic);
typedef void (*LT_OnReceive_Func)(LT_DEVICE_ID deviceId, LT_TOPIC_ID topic, BYTE* value, int valueSize);

struct LittleTalks
{
    LT_OnConnect_Func onConnected_funcPtr;
    LT_OnDisconnect_Func onDisconnected_funcPtr;
    LT_OnSubscribed_Func onSubscribed_funcPtr;
    LT_OnReceive_Func onReceive_funcPtr;

    struct LTDevice* otherDeviceList;
    struct LTTopic* topicsList;

    struct LTDevice* myDevice;
    struct LTUdpSocket* udpSocket;

    int devicesCount;
    BOOL isRunning;

    unsigned char majorVersion;
    unsigned char minorVersion;

    LT_UINT32 tick;
    LT_UINT32 sendIntroductionCounter;
};

void LT_OnIncomingUdpPacket(BYTE* data, int dataSize);

void LT_ProcessReceivedData(BYTE* data, int dataSize);
void LT_ProcessIntroduction(LT_UINT32 deviceIp, BYTE* data, int dataSize);
void LT_ProcessChangeTopic(LT_UINT32 deviceIp, BYTE* data, int dataSize);
void LT_ProcessConfirmation(struct LTDevice* device, BYTE* data, int dataSize);
void LT_SendDisconnect(LT_UINT32 ip);
void LT_SendIntroduction(LT_UINT32 ip, BOOL reconnect);
void LT_SendIntroductionBroadcast();
void LT_ProcessDisconnectDevice(BYTE* data, int length);
void LT_SendConfirmation(struct LTDevice* device, LT_TOPIC_ID topicId, LT_UINT32 topicCounter);
void LT_SendTopic(struct LTTopic* topic, LT_DEVICE_FLAG devicesFlag, BYTE* value, LT_UINT16 valueSize);

void LT_SendRequestTopics(struct LTDevice* device);
int LT_SizeRequestTopics(struct LTDevice* device);
void LT_FillRequestTopics(BYTE* refData, struct LTDevice* device);
struct LTDevice* LT_ProcessRequestTopics(LT_TOPIC_ID** outTopics, LT_UINT16* outTopicsCount, LT_UINT32 deviceIp, BYTE* data, int dataSize);

void LT_SendResponseTopics(struct LTDevice* device, LT_TOPIC_ID* topics, LT_UINT16 topicsCount);
int LT_SizeResponseTopics(LT_TOPIC_ID* topics, LT_UINT16 topicsCount);
void LT_FillResponseTopics(BYTE* refData, LT_TOPIC_ID* topics, LT_UINT16 topicsCount);
struct LTDevice* LT_ProcessResponseTopics(LT_UINT32 deviceIp, BYTE* data, int dataSize);

struct LTTopic* LT_FoundTopic(LT_TOPIC_ID topicId);
struct LTDevice* LT_FoundDevice(LT_DEVICE_ID deviceId);
struct LTDevice* LT_FoundDeviceByIp(LT_UINT32 ip);

BYTE* LT_CreateEmptyPacket(enum LTPacketType packetType, int* refLength);
BOOL LT_CheckPacket(BYTE* data, int dataSize);

void LT_DetectError(enum LTError error, int currCodeLine);

#endif // LITTLETALKS_H
