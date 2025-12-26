#include <cstdio>
#include <cstring>
#include <string>

#if defined(_WIN32) && !defined(_XBOX)
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#define socket_t    SOCKET
#define sockaddr_t  SOCKADDR
#define pcap_dev_name description
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define socket_t    int
#define sockaddr_t  struct sockaddr
#define closesocket close
#define pcap_dev_name name
#endif

#if defined(__HAIKU__)
#include <sys/select.h>
#endif

#ifdef HAVE_PCAP
#include "libui_sdl/LAN_PCap.h"
#include "libui_sdl/LAN_Socket.h"
#endif

#ifdef HAVE_LIBNX
#include <switch/services/bsd.h>
#endif

#ifndef HAVE_WIFI
#define SO_REUSEADDR 0
#define SO_BROADCAST 0
#define socket(domain, type, protocol) NULL
#define bind(sockfd, addr, addrlen) -1
#define setsockopt(sockfd, level, optname, optval, optlen) -1
#define sendto(sockfd, buf, len, flags, dest_addr, addrlen) 0
#define recvfrom(sockfd, buf, len, flags, src_addr, addrlen) 0
#endif

#ifdef HAVE_THREADS
#include <stdlib.h>

#include <rthreads/rthreads.h>
#include <rthreads/rsemaphore.h>
#endif

#include <streams/file_stream.h>
#include <streams/file_stream_transforms.h>
#include <retro_timers.h>

#include "types.h"
#include "utils.h"
#include "Platform.h"

extern char retro_base_directory[4096];

#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (socket_t)-1
#endif

#define NIFI_VER 1

socket_t MPSocket;
sockaddr_t MPSendAddr;
u8 PacketBuffer[2048];

namespace Platform
{
    FILE* OpenFile(const char* path, const char* mode, bool mustexist)
    {
    FILE* ret;

    if (mustexist)
    {
        ret = fopen(path, "rb");
        if (ret) fclose(ret);
        ret = fopen(path, mode);
    }
    else
        ret = fopen(path, mode);

    return ret;
}

   FILE* OpenLocalFile(const char* path, const char* mode)
   {
      std::string fullpath = std::string(retro_base_directory) + std::string(1, PLATFORM_DIR_SEPERATOR) + std::string(path);
      FILE* f = OpenFile(fullpath.c_str(), mode, true);
      return f;
   }

   FILE* OpenDataFile(const char* path)
   {
      return OpenLocalFile(path, "rb");
   }

   void StopEmu()
   {
       return;
   }

   void Semaphore_Reset(Semaphore *sema)
   {
   #ifdef HAVE_THREADS
      while (ssem_get((ssem_t*)sema) > 0) {
        ssem_trywait((ssem_t*)sema);
      }
   #endif
   }

   void Semaphore_Post(Semaphore *sema, int count)
   {
   #ifdef HAVE_THREADS
       for (int i = 0; i < count; i++)
      {
         ssem_signal((ssem_t*)sema);
      }
   #endif
   }

   void Semaphore_Wait(Semaphore *sema)
   {
   #ifdef HAVE_THREADS
      ssem_wait((ssem_t*)sema);
   #endif
   }

   void Semaphore_Free(Semaphore *sema)
   {
   #ifdef HAVE_THREADS
      ssem_t *sem = (ssem_t*)sema;
      if (sem)
         ssem_free(sem);
   #endif
   }

   Semaphore *Semaphore_Create()
   {
   #ifdef HAVE_THREADS
      ssem_t *sem = ssem_new(0);
      if (sem)
         return (Semaphore*)sem;
   #endif
      return NULL;
   }

   Mutex* Mutex_Create()
   {
   #ifdef HAVE_THREADS
      return (Mutex*)slock_new();
   #endif
      return NULL;
   }

   void Mutex_Free(Mutex* mutex)
   {
   #ifdef HAVE_THREADS
      slock_free((slock_t*)mutex);
   #endif
   }

   void Mutex_Lock(Mutex* mutex)
   {
   #ifdef HAVE_THREADS
      slock_lock((slock_t*)mutex);
   #endif
   }

   void Mutex_Unlock(Mutex* mutex)
   {
   #ifdef HAVE_THREADS
      slock_unlock((slock_t*)mutex);
   #endif
   }

   bool Mutex_TryLock(Mutex* mutex)
   {
   #ifdef HAVE_THREADS
      slock_try_lock((slock_t*)mutex);
   #endif
      return true;
   }

   void Thread_Free(Thread *thread)
   {
   #if HAVE_THREADS
      sthread_detach((sthread_t*)thread);
   #endif
   }

   struct ThreadData
   {
      std::function<void()> fn;
   };

   void function_trampoline(void* param) {
      ThreadData* data = (ThreadData*)param;
      data->fn();
      delete data;
   }

   Thread *Thread_Create(std::function<void()> func)
   {
   #if HAVE_THREADS
      return (Thread*) sthread_create(function_trampoline, new ThreadData{func});
   #endif
   }

   void Thread_Wait(Thread *thread)
   {
   #if HAVE_THREADS
      sthread_join((sthread_t*)thread);
   #endif
   }


   bool MP_Init()
   {
      int opt_true = 1;
      int res;

#ifdef _WIN32
      WSADATA wsadata;
      if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
      {
         return false;
      }
#endif // __WXMSW__

      MPSocket = socket(AF_INET, SOCK_DGRAM, 0);
      if (MPSocket < 0)
      {
         return false;
      }

      res = setsockopt(MPSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt_true, sizeof(int));
      if (res < 0)
      {
         closesocket(MPSocket);
         MPSocket = INVALID_SOCKET;
         return false;
      }

      sockaddr_t saddr;
      saddr.sa_family = AF_INET;
      *(u32*)&saddr.sa_data[2] = htonl(INADDR_ANY);
      *(u16*)&saddr.sa_data[0] = htons(7064);
      res = bind(MPSocket, &saddr, sizeof(sockaddr_t));
      if (res < 0)
      {
         closesocket(MPSocket);
         MPSocket = INVALID_SOCKET;
         return false;
      }

      res = setsockopt(MPSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&opt_true, sizeof(int));
      if (res < 0)
      {
         closesocket(MPSocket);
         MPSocket = INVALID_SOCKET;
         return false;
      }

      MPSendAddr.sa_family = AF_INET;
      *(u32*)&MPSendAddr.sa_data[2] = htonl(INADDR_BROADCAST);
      *(u16*)&MPSendAddr.sa_data[0] = htons(7064);

      return true;
   }

   void MP_DeInit()
   {
      if (MPSocket >= 0)
         closesocket(MPSocket);

#ifdef _WIN32
      WSACleanup();
#endif // __WXMSW__
   }

   int MP_SendPacket(u8* data, int len)
   {
      if (MPSocket < 0)
      {
         printf("MP_SendPacket: early return (%d)\n", len);
         return 0;
      }

      if (len > 2048-8)
      {
         printf("MP_SendPacket: error: packet too long (%d)\n", len);
         return 0;
      }

      *(u32*)&PacketBuffer[0] = htonl(0x4946494E); // NIFI
      PacketBuffer[4] = NIFI_VER;
      PacketBuffer[5] = 0;
      *(u16*)&PacketBuffer[6] = htons(len);
      memcpy(&PacketBuffer[8], data, len);

      int slen = sendto(MPSocket, (const char*)PacketBuffer, len+8, 0, &MPSendAddr, sizeof(sockaddr_t));
      if (slen < 8) return 0;
      return slen - 8;

   }

   int MP_RecvPacket(u8* data, bool block)
   {
      if (MPSocket < 0)
      {
         printf("MP_RecvPacket: early return\n");
         return 0;
      }

      fd_set fd;
      struct timeval tv;

      FD_ZERO(&fd);
      FD_SET(MPSocket, &fd);
      tv.tv_sec = 0;
      tv.tv_usec = block ? 5000 : 0;

      if (!select(MPSocket+1, &fd, 0, 0, &tv))
      {
         return 0;
      }

      sockaddr_t fromAddr;
      socklen_t fromLen = sizeof(sockaddr_t);
      int rlen = recvfrom(MPSocket, (char*)PacketBuffer, 2048, 0, &fromAddr, &fromLen);
      if (rlen < 8+24)
      {
         return 0;
      }
      rlen -= 8;

      if (ntohl(*(u32*)&PacketBuffer[0]) != 0x4946494E)
      {
         return 0;
      }

      if (PacketBuffer[4] != NIFI_VER)
      {
         return 0;
      }

      if (ntohs(*(u16*)&PacketBuffer[6]) != rlen)
      {
        return 0;
      }

      memcpy(data, &PacketBuffer[8], rlen);
      return rlen;
   }

   bool LAN_Init()
   {
#ifdef HAVE_PCAP
    if (Config::DirectLAN)
    {
        if (!LAN_PCap::Init(true))
            return false;
    }
    else
    {
        if (!LAN_Socket::Init())
            return false;
    }

    return true;
#else
   return false;
#endif
   }

   void LAN_DeInit()
   {
      // checkme. blarg
      //if (Config::DirectLAN)
      //    LAN_PCap::DeInit();
      //else
      //    LAN_Socket::DeInit();
#ifdef HAVE_PCAP
      LAN_PCap::DeInit();
      LAN_Socket::DeInit();
#endif
   }

   int LAN_SendPacket(u8* data, int len)
   {
#ifdef HAVE_PCAP
      if (Config::DirectLAN)
         return LAN_PCap::SendPacket(data, len);
      else
         return LAN_Socket::SendPacket(data, len);
#else
      return 0;
#endif
   }

   int LAN_RecvPacket(u8* data)
   {
#ifdef HAVE_PCAP
      if (Config::DirectLAN)
         return LAN_PCap::RecvPacket(data);
      else
         return LAN_Socket::RecvPacket(data);
#else
      return 0;
#endif
   }

#ifdef HAVE_OPENGL
   void* GL_GetProcAddress(const char* proc)
   {
      return (void*)nullptr;
   }
#endif

   void Sleep(u64 usecs)
   {
      retro_sleep(usecs / 1000);
   }
};
