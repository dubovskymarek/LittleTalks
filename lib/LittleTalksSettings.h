///
/// @file LittleTalksSettings.h
///
/// @brief Contains macros settings like UDP port, frequence of main loop, max devices count etc.
///
///
#ifndef LITTLETALKSSETTINGS_H
#define LITTLETALKSSETTINGS_H

#define LT_CURR_CODE_LINE        __LINE__

#define LT_TOPICS_MAX_COUNT      65535

#ifndef LT_UDP_PORT
#define LT_UDP_PORT              65021
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

/// \brief Maximum size of udp packet is 1492 with headers. Little talks has max size near this number.
#define LT_UDP_PACKET_SIZE_MAX 1400

#define LT_BROADCAST_KEEPALIVE_AND_INTRODUCTION_INTERVAL    10
#define LT_LOCAL_KEEPALIVE_TIMEOUT_MAX                      60
#define LT_CONFIRMATION_DELAY                               1

#define LT_MAIN_FUNCTION_PERIOD_MS  333

#define LT_PACKET_HEADER_SIZE   20
#define LT_STEP_FREQUENCE       3
#define LT_DEVICES_MAX_COUNT    64

#define LT_TOPIC_ID             LT_UINT64
#define LT_DEVICE_ID            LT_UINT64
#define LT_DEVICE_FLAG          LT_UINT64

#define LT_MAJOR_VERSION        0
#define LT_MINOR_VERSION        5


#endif // LITTLETALKSSETTINGS_H
