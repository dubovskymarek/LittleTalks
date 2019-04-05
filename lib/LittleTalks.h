///
/// @file LittleTalks.h
///
/// @brief LittleTalks library
///
/// Tiny library for client-client communication protocol.
///

#ifndef LITTLETALKS_H
#define LITTLETALKS_H

#include "LittleTalksPrivate.h"

/// \fn BOOL LT_IsInit()
/// \brief LT_IsInit
/// \return true if is initialized, otherwise false
///
BOOL LT_IsInit();

/// \fn BOOL LT_Init(LT_DEVICE_ID deviceId,
///                  LT_OnConnect_Func onConnected_funcPtr,
///                  LT_OnDisconnect_Func onDisconnected_funcPtr,
///                  LT_OnSubscribed_Func onSubscribed_funcPtr,
///                  LT_OnReceive_Func onReceive_funcPtr)
///    \brief Initialization LittleTalks
///
///     Use once before start of communication.
///     Note: If you need restart communication (e.g. on connect to accesspoint), you can use after successfully restart.
///
///     \see LT_Uninit
///
///    \param deviceId Identificatior of this device.
///    \param onConnected_funcPtr Event of connecting new device.
///    \param onDisconnected_funcPtr Event of disconnecting existing device.
///    \param onSubscribed_funcPtr Event of receive subscribed topic from remote device.
///    \param onReceive_funcPtr Event of receive new value of topic.\n
///    \n
///    \brief onReceive_funcPtr:
///    \brief <B>void</B> OnReceive(<B>LT_DEVICE_ID</B> deviceId, <B>LT_TOPIC_ID</B> topic, <B>BYTE*</B> value, <B>int</B> valueLength)\n
///    \brief <B>deviceId</B> - source device, that is publishing topic. \n
///	   \brief <B>topic</B> - id of published topic\n
///	   \brief <B>value</B> - pointer of published topic data\n
///	   \brief <B>valueLength</B> - length of published data\n
///    \n
///    \return state of initialisation
///
BOOL LT_Init(LT_DEVICE_ID deviceId,
             LT_OnConnect_Func onConnected_funcPtr,
             LT_OnDisconnect_Func onDisconnected_funcPtr,
             LT_OnSubscribed_Func onSubscribed_funcPtr,
             LT_OnReceive_Func onReceive_funcPtr);

/// \fn BOOL LT_Init()
///    \brief Uninitialized LittleTalks
///
///     Use once after stop communication.
///     Note: If you need restart communication (e.g. on connect to accesspoint), you can use before restart.
///
///     \see LT_Init
///
///    \return Return if init is success then return true, otwerwise return false
///
void LT_Uninit();

///
/// \brief LT_GetThisDeviceId
/// \return this device id
///
LT_DEVICE_ID LT_GetThisDeviceId();

/// \fn void LT_Start()
/// \brief Start communication
///
/// Note: Required call LT_Init before this calling.
///
/// \see LT_Init
///
void LT_Start();

///
/// \fn void LT_Stop()
/// \brief Stop communication
///
void LT_Stop();

/// \fn void LT_Subscribe(LT_TOPIC_ID topicId, LT_UINT16 valueSizeMax)
///    \brief Subscribe topic
///
///     Start communication on topic. Topic is like virtual file in network, which devices can read and write.
///
///    \param topicId 64-bit unique identificator of topic. Prefer Big-endian bytes coding. Zero id isn't allowed;
///    \param valueSizeMax Maximum size of value buffer.
///
///    \return Return if subscribe is success then return true, otwerwise return false
///
BOOL LT_Subscribe(LT_TOPIC_ID topicId, LT_UINT16 valueSizeMax);

/// \fn void LT_Publish(LT_TOPIC_ID topicId, BYTE* value, LT_UINT16 valueSize)
///    \brief Publish value of topic
///
///     Updates value of topic and synchronises to all subscribed devices.
///
///     \see LT_Subscribe
///
///    \param topicId 64-bit unique identificator of topic.
///    \param valueSize Size of value. It's equal or lower as valueSizeMax parameter of LT_Subscribe.
///
void LT_Publish(LT_TOPIC_ID topicId, BYTE* value, LT_UINT16 valueSize);

/// \fn void LT_RequestForTopics(LT_DEVICE_ID* deviceId)
///    \brief Send request for selected topics to device, that send response values of these topics.
///    \param deviceId device, that will send back value topics
///    \param topics array of requested topics
///    \param topicsCount size of this array
///
void LT_RequestForTopics(LT_DEVICE_ID deviceId, LT_TOPIC_ID* topics, LT_UINT16 topicsCount);

/// \fn BYTE* LT_GetValuePtr(LT_TOPIC_ID topicId)
///    \brief Get value of topic
///
///     Write value to topic. It's like write to virtual file.
///
///    \param topicId 64-bit unique identificator of topic.
///
///    \return Return pointer of value of topic.
///
BYTE* LT_GetValuePtr(LT_TOPIC_ID topicId);

/// \fn LT_UINT16 LT_GetValueSize(LT_TOPIC_ID topicId)
///    \brief Get value of topic
///
///     Write value to topic. It's like write to virtual file.
///
///    \param topicId 64-bit unique identificator of topic.
///
///    \return Return size of value of topic.
///
LT_UINT16 LT_GetValueSize(LT_TOPIC_ID topicId);

///
/// \brief LT_UINT32 LT_GetDeviceIP(LT_DEVICE_ID deviceId)
/// \brief Get IP address of remote connected device
/// \param deviceId remote device
///
/// \return if device is connected then reutnr ip address, otherwise return 0
///
LT_UINT32 LT_GetDeviceIP(LT_DEVICE_ID deviceId);

/// \fn BOOL LT_OnStartMainLoop()
///    \brief Start main loop.
///
///     Use before start main loop.
///
///     \see LT_OnStepMainLoop
///     \see LT_OnEndMainLoop
///
///    \return Return if start main loop is success then return true, otwerwise return false
///
BOOL LT_OnStartMainLoop();

/// \fn void LT_OnStepMainLoop()
///    \brief Step of main loop.
///
///     Use every 333 miliseconds.
///
///     \see LT_OnStartMainLoop
///     \see LT_OnEndMainLoop
///
void LT_OnStepMainLoop();

/// \fn void LT_OnEndMainLoop()
///    \brief End main loop.
///
///     Use after end of main loop.
///
///     \see LT_OnStartMainLoop
///     \see LT_OnStepMainLoop
///
void LT_OnEndMainLoop();

///
/// \brief BOOL LT_ForceSubscribeRemoteTopic(LT_DEVICE_ID deviceId, LT_TOPIC_ID topicId)
/// \brief It's special advanced function for immediately connect remote device and subscribe remote topic.
/// This function use only situation, when you cannot wait for standard connection (10-20 seconds), but you need
/// send topic in few miliseconds. For example: Android device is in sleep mode, every communication is stoped, and sometimes
/// every 5 minutes i call small function. Android cannot wake up but it's allow me call small function few miliseconds,
/// but every devices are disconnected, and  they need 10-20 seconds to reconnect (this time it's not supported by sleeping android).
/// To resolve this problem, is ignore standard connection process and force this process at one moment.
///
/// example using:
/// LT_ForceSubscribeRemoteTopic(remoteDeviceId, deviceIp, topicId);
/// LT_Publish(topicId, valuePtr, valueSize);
///
/// \param deviceId remote device id
/// \param topicId remote topic
///
///    \return Return if force subscribe is success then return true, otwerwise return false
///
BOOL LT_ForceSubscribeRemoteTopic(LT_DEVICE_ID deviceId, LT_TOPIC_ID topicId);

///
/// \brief LT_AddDevice
/// \brief Special function for add device, that won't auto release after disconnected.
/// \brief LittleTalks alloc device structure after device connected, and release after disconnected.
/// \brief Maximum connected devices is 64, and if you need long-time live device, what you need listen, use this function.
///
void LT_AddDevice(LT_DEVICE_ID deviceId);

#endif // LITTLETALKS_H
