///
/// @file LTTcpUdpAdapter.h
///
/// @brief Abstract adapter
///
/// Adapter for custom implementation of LittleTalks library. Use for current platform (e.g. QT like in this current file).
/// You can change LTTcpServer, LTUdpSocket structures, but all header functions let without changing.
///

#ifndef LTPLATFORMADAPTER_H
#define LTPLATFORMADAPTER_H

#include "LittleTalksSettings.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include<winsock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include <stdio.h>
#include <stdlib.h>

typedef unsigned char       BYTE     ;
typedef unsigned char       LT_UINT8 ;
typedef unsigned short      LT_UINT16;
typedef unsigned int        LT_UINT32;
typedef unsigned long long  LT_UINT64;

#ifndef BOOL
#define BOOL unsigned char
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

void* LT_MALLOC(unsigned int size);
void LT_FREE(void* array);

void LT_MICROSLEEP(LT_UINT64 time);

typedef void (*LT_OnUdpIncomingPacket_Func)(BYTE* data, int dataSize);

typedef BOOL (*LT_OnStartMainLoop_Func)();
typedef void (*LT_OnStepMainLoop_Func)();
typedef void (*LT_OnEndMainLoop_Func)();

///
/// \struct LTUdpSocket
///
/// One instance of udp socket, that use for send introduction packet.
///
struct LTUdpSocket
{
#ifdef _WIN32
    SOCKET socket;
#else
    int socket;
#endif
};

/// \fn void LTPlatformAdapter_Init(LT_OnUdpIncomingPacket_Func udpIncomingPacket_funcPtr)
///    \brief Initialization
///
///     Called once on LT_Init.
///
///     \see LTPlatformAdapter_Uninit
///     \see LTPlatformAdapter_Start
///     \see LT_Init
///
///    \param udpIncomingPacket_funcPtr Event of incoming udp packet. Use only to receive introduction packet.
///
void LTPlatformAdapter_Init(LT_OnUdpIncomingPacket_Func udpIncomingPacket_funcPtr);

/// \fn void LTPlatformAdapter_Uninit()
///    \brief Initialization
///
///     Called once on LT_Uninit.
///
///     \see LT_Uninit
///
void LTPlatformAdapter_Uninit();

/// \fn void LTPlatformAdapter_Lock()
///    \brief Lock critical memory in multicore CPU. In one core CPU this function has empty definition.
///
///     \see LTPlatformAdapter_Unlock
///
void LTPlatformAdapter_Lock();

/// \fn void LTPlatformAdapter_Unlock()
///    \brief Unlock critical memory in multicore CPU. In one core CPU this function has empty definition.
///
///     \see LTPlatformAdapter_Lock
///
void LTPlatformAdapter_Unlock();

/// \fn void LTPlatformAdapter_Start()
/// \brief Start udp communication
///
/// Called once on LT_Start.
///
/// \see LTPlatformAdapter_Stop
/// \see LT_Start
///
void LTPlatformAdapter_Start();

///
/// \fn void  LTPlatformAdapter_Stop()
/// \brief Stop udp communication
///
/// Called once on LT_Stop
///
/// \see LTPlatformAdapter_Stop
/// \see LT_Stop
///
void LTPlatformAdapter_Stop();

/// \fn void LTPlatformAdapter_UDP_Init(LTUdpSocket* udpSocket)
///    \brief Initialization of UDP socket
///
///     Called on LT_OnStartMainLoop. Use only for sending/receiving Introduction packet.
///
///    \param udpSocket UDP socket, that need self initialization.
///    \param tcpServer use only for special platform.
///
void LTPlatformAdapter_UDP_Init(struct LTUdpSocket* udpSocket);

/// \fn void LTPlatformAdapter_UDP_Uninit(LTUdpSocket* udpSocket)
///    \brief Uninitialization of UDP socket
///
///     Called on LT_OnEndMainLoop.
///     NOTE: Don't release (free(udpSocket);). Free of udpSocket is called in LT_OnEndMainLoop.
///
///    \param udpSocket UDP socket, that need self uninitialization.
///
void LTPlatformAdapter_UDP_Uninit(struct LTUdpSocket* udpSocket);

/// \fn void LTPlatformAdapter_UDP_Bind(LTUdpSocket* udpSocket, int port)
///    \brief UDP socket bind to udp port
///
///     Called on LT_OnEndMainLoop.
///
///    \param udpSocket binding UDP socket.
///    \param port UDP port.
///
void LTPlatformAdapter_UDP_Bind(struct LTUdpSocket* udpSocket, int port);

/// \fn void LTPlatformAdapter_UDP_Send(LTUdpSocket* udpSocket, LT_UINT32 ip, int port, BYTE* data, int dataSize)
///    \brief Send UDP socket
///
///     Called on LT_SendIntroduction. Send introduction packet.
///
///    \param udpSocket UDP socket, that try send data.
///    \param ip destination id
///    \param port UDP port
///    \param data sending data
///    \param dataSize sending data size
///
void LTPlatformAdapter_UDP_Send(struct LTUdpSocket* udpSocket, LT_UINT32 ip, int port, BYTE* data, int dataSize);

/// \fn LT_UINT32 LTPlatformAdapter_GetIP()
///    \brief Get IP address
///
///    \return return your IP address
///
LT_UINT32 LTPlatformAdapter_GetIP();

/// \fn LT_UINT32 LTPlatformAdapter_GetGateway()
///    \brief Get gateway address.
///
///    \return return gateway IP
///
LT_UINT32 LTPlatformAdapter_GetGateway();

/// \fn LT_UINT32 LTPlatformAdapter_GetNetworkMask()
///    \brief Get IP mask of network.
///
///    \return return network mask
///
LT_UINT32 LTPlatformAdapter_GetNetworkMask();

#endif // LTPLATFORMADAPTER_H
