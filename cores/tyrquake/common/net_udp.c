/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "compat/strl.h"
#include <stdint.h>
#include <sys/types.h>
#if !defined(_MSC_VER) && !defined(__PS3__)
#include <sys/param.h>
#endif

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#if !defined(__PSL1GHT__) && defined(__PS3__)
#include <netex/errno.h>
#endif

#include <net/net_compat.h>
#include <net/net_socket.h>

#include "common.h"
#include "console.h"
#include "net.h"
#include "net_udp.h"
#include "quakedef.h"
#include "sys.h"

#if defined(VITA)
#ifndef in_addr
struct in_addr {
    unsigned long s_addr;  /* load with inet_aton() */
};
#endif
#endif

#ifdef VITA
#define IPPROTO_UDP SCE_NET_IPPROTO_UDP
#define INADDR_BROADCAST SCE_NET_INADDR_BROADCAST
#define SO_BROADCAST SCE_NET_SO_BROADCAST
#endif

#ifdef _3DS
#define SO_BROADCAST 0x20
#endif

#ifndef INADDR_NONE
#define INADDR_NONE    ((uint32_t) 0xffffffff)  /* 255.255.255.255 */
#endif

#ifndef INADDR_LOOPBACK
#define	INADDR_LOOPBACK		0x7f000001	
#endif

/* socket for fielding new connections */
static int net_acceptsocket = -1;
static int net_controlsocket = -1;
static int net_broadcastsocket = 0;
static netadr_t broadcastaddr;

/*
 * There are three addresses that we may use in different ways:
 *   myAddr	- This is the "default" address returned by the OS
 *   localAddr	- This is an address to advertise in CCREP_SERVER_INFO
 *		 and CCREP_ACCEPT response packets, rather than the
 *		 default address (sometimes the default address is not
 *		 suitable for LAN clients; i.e. loopback address). Set
 *		 on the command line using the "-localip" option.
 *   bindAddr	- The address to which we bind our network socket. The
 *		 default is INADDR_ANY, but in some cases we may want
 *		 to only listen on a particular address. Set on the
 *		 command line using the "-ip" option.
 */
#ifndef IF_NAMESIZE
#define IF_NAMESIZE 16
#define MAXHOSTNAMELEN 64
#endif
static netadr_t myAddr;
static netadr_t localAddr;
static netadr_t bindAddr;
static char ifname[IF_NAMESIZE];


static void
NetadrToSockadr(const netadr_t *a, struct sockaddr_in *s)
{
    memset(s, 0, sizeof(*s));
    s->sin_family = AF_INET;

    s->sin_addr.s_addr = a->ip.l;
    s->sin_port = a->port;
}

static void
SockadrToNetadr(const struct sockaddr_in *s, netadr_t *a)
{
    a->ip.l = s->sin_addr.s_addr;
    a->port = s->sin_port;
}

int
UDP_Init(void)
{
   int i;
   int err;
   char buff[MAXHOSTNAMELEN];
   char *colon;
   struct hostent *local;
   netadr_t addr;

   if (COM_CheckParm("-noudp"))
      return -1;

   network_init();

   /* determine my name & address, default to loopback */
   myAddr.ip.l = htonl(INADDR_LOOPBACK);
   myAddr.port = htons(DEFAULTnet_hostport);
#if defined(__PS3__)
#elif defined (VITA)
   SceNetCtlInfo info;
   sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info);
   sceNetInetPton(SCE_NET_AF_INET, info.ip_address, &myAddr.ip.l);
#else
   err = gethostname(buff, MAXHOSTNAMELEN);
   if (err) {
      Con_Printf("%s: WARNING: gethostname failed (%s)\n", __func__,
            strerror(errno));
   } else {
      buff[MAXHOSTNAMELEN - 1] = 0;
      local = gethostbyname(buff);
      if (!local) {
         Con_Printf("%s: WARNING: gethostbyname failed\n", __func__);
      } else if (local->h_addrtype != AF_INET) {
         Con_Printf("%s: address from gethostbyname not IPv4\n", __func__);
      } else {
         struct in_addr *inaddr = (struct in_addr *)local->h_addr_list[0];
         myAddr.ip.l = inaddr->s_addr;
      }
   }
#endif
   i = COM_CheckParm("-ip");
   if (i && i < com_argc - 1) {
      bindAddr.ip.l = inet_addr(com_argv[i + 1]);
      if (bindAddr.ip.l == INADDR_NONE)
         Sys_Error("%s: %s is not a valid IP address", __func__,
               com_argv[i + 1]);
      Con_Printf("Binding to IP Interface Address of %s\n", com_argv[i + 1]);
   } else {
      bindAddr.ip.l = INADDR_NONE;
   }

   i = COM_CheckParm("-localip");
   if (i && i < com_argc - 1) {
      localAddr.ip.l = inet_addr(com_argv[i + 1]);
      if (localAddr.ip.l == INADDR_NONE)
         Sys_Error("%s: %s is not a valid IP address", __func__,
               com_argv[i + 1]);
      Con_Printf("Advertising %s as the local IP in response packets\n",
            com_argv[i + 1]);
   } else {
      localAddr.ip.l = INADDR_NONE;
   }

   net_controlsocket = UDP_OpenSocket(0);
   if (net_controlsocket == -1) {
      Con_Printf("%s: Unable to open control socket, UDP disabled\n",
            __func__);
      return -1;
   }

   /* myAddr may resolve to 127.0.0.1, see if we can do any better */
   memset (ifname, 0, sizeof(ifname));
   if (ifname[0] == 0) {
      Con_Printf ("UDP, Local address: %s\n", NET_AdrToString(&myAddr));
   }

   broadcastaddr.ip.l = INADDR_BROADCAST;
   broadcastaddr.port = htons(net_hostport);

   UDP_GetSocketAddr(net_controlsocket, &addr);
   strlcpy(my_tcpip_address, NET_AdrToString(&addr), sizeof(my_tcpip_address));
   colon = strrchr(my_tcpip_address, ':');
   if (colon)
      *colon = 0;

   Con_Printf("UDP Initialized (%s)\n", my_tcpip_address);
   tcpipAvailable = true;

   return net_controlsocket;
}


void
UDP_Shutdown(void)
{
    UDP_Listen(false);
    UDP_CloseSocket(net_controlsocket);

    /* Reset the file-static fd values so a subsequent UDP_Init
     * after a deinit/reinit cycle (statically-linked target)
     * starts from a known-clean state.  Without this, the stale
     * fd values may be re-read by UDP_Listen(true) before
     * UDP_Init re-opens them, with various ill effects depending
     * on what file the host process has reassigned that fd to. */
    net_controlsocket   = -1;
    net_acceptsocket    = -1;
    net_broadcastsocket = 0;
    tcpipAvailable      = false;
}


void
UDP_Listen(qboolean state)
{
    /* enable listening */
    if (state) {
	if (net_acceptsocket != -1)
	    return;
	if ((net_acceptsocket = UDP_OpenSocket(net_hostport)) == -1)
	    Sys_Error("%s: Unable to open accept socket", __func__);
	return;
    }
    /* disable listening */
    if (net_acceptsocket == -1)
	return;
    UDP_CloseSocket(net_acceptsocket);
    net_acceptsocket = -1;
}


int
UDP_OpenSocket(int port)
{
   int newsocket;
   struct sockaddr_in address;

   if ((newsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
      return -1;
   socket_nonblock(newsocket);

   address.sin_family = AF_INET;
   if (bindAddr.ip.l != INADDR_NONE)
      address.sin_addr.s_addr = bindAddr.ip.l;
   else
      address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons((unsigned short)port);
   if (bind(newsocket, (struct sockaddr *)&address, sizeof(address)) == -1)
      goto ErrorReturn;

   return newsocket;

ErrorReturn:
   close(newsocket);
   return -1;
}


int
UDP_CloseSocket(int socket)
{
    if (socket == net_broadcastsocket)
	net_broadcastsocket = 0;
    return close(socket);
}


int
UDP_CheckNewConnections(void)
{
   struct sockaddr_in from;
   socklen_t fromlen;
   char buff[1];

   if (net_acceptsocket == -1)
      return -1;

   /* quietly absorb empty packets */
   if (recvfrom (net_acceptsocket, buff, 0, 0, (struct sockaddr *)&from, &fromlen) >= 0)
      return net_acceptsocket;
   return -1;
}


int
UDP_Read(int socket, void *buf, int len, netadr_t *addr)
{
    struct sockaddr_in saddr;
    socklen_t addrlen = sizeof(saddr);
    int ret;

    ret = recvfrom(socket, buf, len, 0, (struct sockaddr *)&saddr, &addrlen);
    SockadrToNetadr(&saddr, addr);
    if (isagain(ret))
	return 0;
    return ret;
}


static int
UDP_MakeSocketBroadcastCapable(int socket)
{
   char i = 1;

   /* make this socket broadcast capable */
   if (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &i, sizeof(i)) < 0)
      return -1;
   net_broadcastsocket = socket;

   return 0;
}


int
UDP_Broadcast(int socket, const void *buf, int len)
{
    int ret;

    if (socket != net_broadcastsocket) {
	if (net_broadcastsocket != 0)
	    Sys_Error("Attempted to use multiple broadcasts sockets");
	ret = UDP_MakeSocketBroadcastCapable(socket);
	if (ret == -1) {
	    Con_Printf("Unable to make socket broadcast capable\n");
	    return ret;
	}
    }

    return UDP_Write(socket, buf, len, &broadcastaddr);
}


int
UDP_Write(int socket, const void *buf, int len, const netadr_t *addr)
{
    struct sockaddr_in saddr;
    int ret;

    NetadrToSockadr(addr, &saddr);
    ret = sendto(socket, buf, len, 0, (struct sockaddr *)&saddr, sizeof(saddr));
    if (isagain(ret))
	return 0;
    return ret;
}


int
UDP_GetSocketAddr(int socket, netadr_t *addr)
{
    struct sockaddr_in saddr;
    socklen_t len = sizeof(saddr);

    memset(&saddr, 0, len);
    getsockname(socket, (struct sockaddr *)&saddr, &len);

#ifdef VITA
	unsigned int a, tmp;
	a = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
	sceNetInetPton(SCE_NET_AF_INET, "127.0.0.1", &tmp);
	if (a == 0 || a == tmp)
		memcpy(&((struct sockaddr_in *)addr)->sin_addr.s_addr, &myAddr, sizeof(unsigned int));
#else
	 /*
     * The returned IP is embedded in our repsonse to a broadcast request for
     * server info from clients. The server admin may wish to advertise a
     * specific IP for various reasons, so allow the "default" address
     * returned by the OS to be overridden.
     */
    if (localAddr.ip.l != INADDR_NONE)
	saddr.sin_addr.s_addr = localAddr.ip.l;
    else {
	struct in_addr a = saddr.sin_addr;
	if (!a.s_addr || a.s_addr == htonl(INADDR_LOOPBACK))
	    saddr.sin_addr.s_addr = myAddr.ip.l;
    }
    SockadrToNetadr(&saddr, addr);
#endif

    return 0;
}


int
UDP_GetNameFromAddr(const netadr_t *addr, char *name)
{
    struct hostent *hostentry;
#ifndef VITA
    hostentry = gethostbyaddr((const char*)&addr->ip.l, sizeof(addr->ip.l), AF_INET);
    if (hostentry) {
	strlcpy(name, (char *)hostentry->h_name, NET_NAMELEN);
	return 0;
    }
#endif
    /* The 'name' buffer is by convention NET_NAMELEN bytes
     * (see hostcache_t.cname / qsocket_t.address); callers
     * either pass one of those struct fields or a stack
     * buffer of the same size.  strlcpy with NET_NAMELEN
     * matches the implicit interface contract. */
    strlcpy(name, NET_AdrToString(addr), NET_NAMELEN);

    return 0;
}


int
UDP_GetAddrFromName(const char *name, netadr_t *addr)
{
    struct hostent *hostentry;

    if (name[0] >= '0' && name[0] <= '9')
	return NET_PartialIPAddress(name, &myAddr, addr);

    hostentry = gethostbyname(name);
    if (!hostentry)
	return -1;

    /* memcpy avoids the strict-aliasing violation of dereferencing
     * h_addr_list[0] (a char *) through an int *. */
    memcpy(&addr->ip.l, hostentry->h_addr_list[0], sizeof(addr->ip.l));
    addr->port = htons(net_hostport);

    return 0;
}


int
UDP_GetDefaultMTU(void)
{
    return 1400;
}
