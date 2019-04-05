#include "LTPlatformAdapter.h"

#include "LittleTalks.h"

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
//#pragma comment(lib,"ws2_32.lib") //Winsock Library
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#endif

#ifdef _WIN32
WSADATA g_wsa;
BOOL g_wsaIsInitialized = FALSE;

BOOL IsWSAInitialized()
{
    return g_wsaIsInitialized;
}
void WSAInit()
{
    if(WSAStartup(MAKEWORD(2, 2), &g_wsa) != 0)
        LT_DetectError(LTError_UnknownError, LT_CURR_CODE_LINE);
}
void WSAUninit()
{
    WSACleanup();
}

#endif

struct LT_Mutex
{
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutex2;
    int sharedResource;
    int recursionCounter;
};

static pthread_t g_lt_mainThread;
static pthread_t g_lt_udpReceiveThread;

static BOOL g_lt_mainThread_isRunning = FALSE;
static BOOL g_lt_udpReceiveThread_isRunning = FALSE;

static struct LT_Mutex g_lt_mutex2;

static LT_UINT32 g_lt_ip = 0;
static LT_UINT32 g_lt_netmask = 0;

static LT_OnUdpIncomingPacket_Func g_udpIncomingPacket_funcPtr = NULL;

extern struct LittleTalks* g_littleTalks;

void* LT_MALLOC(unsigned int size)
{
    void* data = malloc(size);

    return data;
}
void LT_FREE(void* data)
{
    free(data);
}

void LT_MICROSLEEP(LT_UINT64 time)
{
#ifdef _WIN32
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(time * 10); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
#else
    usleep((useconds_t)time);
#endif
}

void* LT_main_func(void* ptr);
void* LT_udp_rec_func(void* ptr);

void* LT_main_func(void* ptr)
{
    UNUSED(ptr);

    int recRes;

    LT_OnStartMainLoop();

    recRes = pthread_create(&g_lt_udpReceiveThread, NULL, LT_udp_rec_func, (void*)NULL);
    UNUSED(recRes);

    g_lt_mainThread_isRunning = TRUE;

    ///Period 333 ms
    LT_UINT64 step = 333333;
    while(g_lt_mainThread_isRunning)
    {
        if(g_littleTalks == NULL)
            break;

        if(!g_littleTalks->isRunning)
        {
            LT_MICROSLEEP(step);
            continue;
        }

        LT_OnStepMainLoop();

        LT_MICROSLEEP(step);
    }

    g_lt_udpReceiveThread_isRunning = FALSE;

    pthread_join(g_lt_udpReceiveThread, NULL);

    LT_OnEndMainLoop();

    return NULL;
}
void* LT_udp_rec_func(void* ptr)
{
    UNUSED(ptr);

#ifdef _WIN32
    int slen = sizeof(struct sockaddr);
#else
    socklen_t slen = sizeof(struct sockaddr);
#endif
    struct sockaddr_in addr;
    ssize_t recv_len;
    char buf[1500];
    int socket;

    g_lt_udpReceiveThread_isRunning = TRUE;

    while(g_lt_udpReceiveThread_isRunning)
    {
        if(g_littleTalks == NULL)
            break;

        LTPlatformAdapter_Lock();
        if(!g_littleTalks->isRunning)
        {
            LTPlatformAdapter_Unlock();
            LT_MICROSLEEP(10000);
            continue;
        }

        if(g_littleTalks->udpSocket == NULL)
        {
            LTPlatformAdapter_Unlock();
            LT_MICROSLEEP(10000);
            continue;
        }

        socket = g_littleTalks->udpSocket->socket;
        UNUSED(socket);
        LTPlatformAdapter_Unlock();

        if ((recv_len = recvfrom(g_littleTalks->udpSocket->socket, buf, 1500, 0, (struct sockaddr*)&addr, &slen)) == -1)
        {
            LTPlatformAdapter_Lock();
            LT_DetectError(LTError_UnknownError, LT_CURR_CODE_LINE);
            LTPlatformAdapter_Unlock();
            LT_MICROSLEEP(1000000);
            continue;
        }

        LTPlatformAdapter_Lock();
        g_udpIncomingPacket_funcPtr((BYTE*)buf, (int)recv_len);
        LTPlatformAdapter_Unlock();
    }

    return NULL;
}

void LTPlatformAdapter_Init(LT_OnUdpIncomingPacket_Func udpIncomingPacket_funcPtr)
{
    int mainRes;

#ifdef _WIN32
    if(!IsWSAInitialized())
        WSAInit();
#endif

    pthread_mutexattr_init(&g_lt_mutex2.mutex2);
    pthread_mutexattr_setpshared(&g_lt_mutex2.mutex2, PTHREAD_PROCESS_SHARED);

    g_lt_mutex2.sharedResource = 0;
    g_lt_mutex2.recursionCounter = 0;
    pthread_mutex_init(&(g_lt_mutex2.mutex), &g_lt_mutex2.mutex2);

    g_udpIncomingPacket_funcPtr = udpIncomingPacket_funcPtr;

    mainRes = pthread_create(&g_lt_mainThread, NULL, LT_main_func, (void*)NULL);
    UNUSED(mainRes);

    while(!g_lt_udpReceiveThread_isRunning)
    {
        LT_MICROSLEEP(10000);
    }
}
void LTPlatformAdapter_Uninit()
{
     LTPlatformAdapter_Stop();

    g_lt_mainThread_isRunning = FALSE;
    pthread_join(g_lt_mainThread, NULL);

    g_udpIncomingPacket_funcPtr = NULL;

#ifdef _WIN32
    if(IsWSAInitialized())
        WSAUninit();
#endif
}

void LTPlatformAdapter_Lock()
{
    if(g_lt_mutex2.recursionCounter == 0)
        pthread_mutex_lock(&(g_lt_mutex2.mutex));
    g_lt_mutex2.recursionCounter++;
}
void LTPlatformAdapter_Unlock()
{
    g_lt_mutex2.recursionCounter--;
    if(g_lt_mutex2.recursionCounter == 0)
        pthread_mutex_unlock(&(g_lt_mutex2.mutex));
}
void LTPlatformAdapter_Start()
{
}
void LTPlatformAdapter_Stop()
{
}

void LTPlatformAdapter_UDP_Init(struct LTUdpSocket* udpSocket)
{
#ifdef _WIN32
    if(!IsWSAInitialized())
        WSAInit();
#endif

    memset((char*)udpSocket, 0, sizeof(struct LTUdpSocket));
}
void LTPlatformAdapter_UDP_Uninit(struct LTUdpSocket* udpSocket)
{
#ifdef _WIN32
    closesocket(udpSocket->socket);
#endif

    memset((char*)udpSocket, 0, sizeof(struct LTUdpSocket));
}
void LTPlatformAdapter_UDP_Bind(struct LTUdpSocket* udpSocket, int port)
{
#ifdef _WIN32
    if((udpSocket->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
#else
    if((udpSocket->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
#endif
    {
        LT_DetectError(LTError_UnknownError, LT_CURR_CODE_LINE);

        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(udpSocket->socket, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
    {
        LT_DetectError(LTError_UnknownError, LT_CURR_CODE_LINE);

        return;
    }
}
void LTPlatformAdapter_UDP_Send(struct LTUdpSocket* udpSocket, LT_UINT32 ip, int port, BYTE* data, int dataSize)
{
    int on;
    struct sockaddr_in addr;
    ssize_t w;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip);//0xff64a8c0;

    on = 1;
    setsockopt(udpSocket->socket, SOL_SOCKET, SO_BROADCAST, (char*)&on, sizeof(on));

    w = sendto(udpSocket->socket, (char*)data, (size_t)dataSize, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
//    printf("sendto to %x %d %d\n", addr.sin_addr.s_addr, dataSize, w);fflush(stdout);

    if(w == -1)
    {
        LT_DetectError(LTError_UnknownError, LT_CURR_CODE_LINE);
    }
}


LT_UINT32 LTPlatformAdapter_GetIP()
{
#ifdef _WIN32
    char name[255];
    struct in_addr* addr;
    PHOSTENT hostinfo;

    if(!IsWSAInitialized())
        WSAInit();

    if(gethostname(name, sizeof(name)) == 0)
    {
        if((hostinfo = gethostbyname(name)) != NULL)
        {
            addr = (struct in_addr*)*hostinfo->h_addr_list;

        //    printf("%x", addr->S_un.S_addr);fflush(stdout);
            g_lt_ip = htonl(addr->S_un.S_addr);
            g_lt_netmask = htonl(addr->S_un.S_addr & 0x00ffffff);
        }
    }
#else
    struct ifaddrs* ifaddr;
    struct ifaddrs* ifa;
    struct sockaddr_in* ip_address;
    struct sockaddr_in* ip_netmask;
//    int s;
//    char host[NI_MAXHOST];

    if(g_lt_ip == 0)
    {
        if (getifaddrs(&ifaddr) == -1)
        {
            LT_DetectError(LTError_UnknownError, LT_CURR_CODE_LINE);
            perror("getifaddrs");
            exit(EXIT_FAILURE);
        }

        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
                continue;

        //    s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

            if((strcmp(ifa->ifa_name, "wlan0") == 0 || strcmp(ifa->ifa_name, "en0") == 0) && (ifa->ifa_addr->sa_family==AF_INET))
            {
            //    if (s != 0)
            //    {
            //        LT_DetectError(LTError_UnknownError, LT_CURR_CODE_LINE);
            //        exit(EXIT_FAILURE);
            //    }

                ip_address = (struct sockaddr_in*)ifa->ifa_addr;
                ip_netmask = (struct sockaddr_in*)ifa->ifa_netmask;

                g_lt_ip = htonl(ip_address->sin_addr.s_addr);
                g_lt_netmask = htonl(ip_netmask->sin_addr.s_addr);
            }
        }

        freeifaddrs(ifaddr);
    }
#endif

    return g_lt_ip;
}

LT_UINT32 LTPlatformAdapter_GetGateway()
{
    return LTPlatformAdapter_GetIP() & LTPlatformAdapter_GetNetworkMask();
}

LT_UINT32 LTPlatformAdapter_GetNetworkMask()
{
    return g_lt_netmask;
}

