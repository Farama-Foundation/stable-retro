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

extern int m_state;

#if defined(_WIN32)
#include <winsock2.h>
#include <windows.h>
#else
#include <sys/types.h>
#endif
#if !defined(__PSL1GHT__) && defined(__PS3__)
#include <netex/errno.h>
#endif
#include <net/net_compat.h>
#include <net/net_socket.h>

#include "compat/strl.h"

#include "cmd.h"
#include "common.h"
#include "console.h"
#include "keys.h"
#include "menu.h"
#include "net.h"
#include "net_dgrm.h"
#include "quakedef.h"
#include "server.h"
#include "screen.h"
#include "sys.h"

/* statistic counters */
static int packetsSent = 0;
static int packetsReSent = 0;
static int packetsReceived = 0;
static int receivedDuplicateCount = 0;
static int shortPacketCount = 0;
static int droppedDatagrams;

static net_driver_t *dgrm_driver;

static struct {
    unsigned int length;
    unsigned int sequence;
    byte data[NET_MAXMESSAGE];
} packetBuffer;

#ifdef DEBUG
static const char *
StrAddr(netadr_t *addr)
{
    static char buf[32];

    snprintf(buf, sizeof(buf), "%d.%d.%d.%d:%d",
	    addr->ip.b[0], addr->ip.b[1], addr->ip.b[2], addr->ip.b[3],
	    ntohs(addr->port));
    return buf;
}
#endif

/* The outer struct has a union as its first member; explicit braces
 * around the union initializer keep GCC 15's -Wmissing-braces quiet. */
static netadr_t banAddr = { { INADDR_ANY  }, 0, 0 };
static netadr_t banMask = { { INADDR_NONE }, 0, 0 };

static void
NET_Ban_f(void)
{
   char addrStr[32];
   char maskStr[32];
   void (*print)(const char *fmt, ...);

   if (cmd_source == src_command) {
      if (!sv.active) {
         Cmd_ForwardToServer();
         return;
      }
      print = Con_Printf;
   }
   else
   {
      if (pr_global_struct->deathmatch)
         return;
      print = SV_ClientPrintf;
   }

   switch (Cmd_Argc())
   {
      case 1:
         if (banAddr.ip.l != INADDR_ANY)
         {
            strlcpy(addrStr, NET_AdrToString(&banAddr), sizeof(addrStr));
            strlcpy(maskStr, NET_AdrToString(&banMask), sizeof(maskStr));
            print("Banning %s [%s]\n", addrStr, maskStr);
         } else
            print("Banning not active\n");
         break;

      case 2:
         if (strcasecmp(Cmd_Argv(1), "off") == 0)
            banAddr.ip.l = INADDR_ANY;
         else
            banAddr.ip.l = inet_addr(Cmd_Argv(1));
         banMask.ip.l = INADDR_NONE;
         break;

      case 3:
         banAddr.ip.l = inet_addr(Cmd_Argv(1));
         banMask.ip.l = inet_addr(Cmd_Argv(2));
         break;

      default:
         print("BAN ip_address [mask]\n");
         break;
   }
}

static int
SendPacket(qsocket_t *sock)
{
    unsigned int packetLen;
    unsigned int dataLen;
    unsigned int eom;

    if (sock->sendMessageLength <= sock->mtu) {
	dataLen = sock->sendMessageLength;
	eom = NETFLAG_EOM;
    } else {
	dataLen = sock->mtu;
	eom = 0;
    }
    packetLen = NET_HEADERSIZE + dataLen;

    packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
    packetBuffer.sequence = BigLong(sock->sendSequence++);
    memcpy(packetBuffer.data, sock->sendMessage, dataLen);

    if (sock->landriver->Write(sock->socket, &packetBuffer, packetLen,
			       &sock->addr) == -1)
	return -1;

    sock->lastSendTime = net_time;
    packetsSent++;

    return 1;
}

int
Datagram_SendMessage(qsocket_t *sock, sizebuf_t *data)
{
#ifdef DEBUG
    if (data->cursize == 0)
	Sys_Error("%s: zero length message", __func__);

    if (sock->canSend == false)
	Sys_Error("%s: called with canSend == false", __func__);
#endif

    /* This one stays out of the DEBUG gate.  The other two
     * assertions above are protocol-state sanity checks;
     * this one guards a memcpy into sock->sendMessage,
     * which is sized NET_MAXMESSAGE.  Currently the engine
     * keeps every sizebuf_t passed in here at maxsize <=
     * NET_MAXMESSAGE (SZ_GetSpace enforces cursize <=
     * maxsize, so the precondition holds transitively), so
     * this never fires in well-behaved code.  But if a
     * future change ever wires a larger sizebuf through
     * here -- or if SZ_GetSpace's invariant gets broken
     * upstream -- the memcpy below overruns sock->
     * sendMessage straight into adjacent qsocket_t fields
     * (sendMessageLength, mtu, the function-pointer ack
     * accounting, etc.). Crash loudly rather than corrupt
     * silently. */
    if (data->cursize > NET_MAXMESSAGE)
	Sys_Error("%s: message too big %u", __func__, data->cursize);

    memcpy(sock->sendMessage, data->data, data->cursize);
    sock->sendMessageLength = data->cursize;
    sock->canSend = false;

    return SendPacket(sock);
}

static int
SendMessageNext(qsocket_t *sock)
{
    sock->sendNext = false;

    return SendPacket(sock);
}

static int
ReSendMessage(qsocket_t *sock)
{
    sock->sendNext = false;

    return SendPacket(sock);
}


qboolean
Datagram_CanSendMessage(qsocket_t *sock)
{
    if (sock->sendNext)
	SendMessageNext(sock);

    return sock->canSend;
}


qboolean
Datagram_CanSendUnreliableMessage(qsocket_t *sock)
{
    return true;
}


int
Datagram_SendUnreliableMessage(qsocket_t *sock, sizebuf_t *data)
{
    int packetLen;

#ifdef DEBUG
    if (data->cursize == 0)
	Sys_Error("%s: zero length message", __func__);

    if (data->cursize > sock->mtu)
	Sys_Error("%s: message too big %u", __func__, data->cursize);
#endif

    /* Memory bound on the unreliable path.  See Datagram_
     * SendMessage for the full reasoning; the same memcpy-
     * past-buffer-end concern applies here but against the
     * shared packetBuffer.data (sized NET_MAXMESSAGE) rather
     * than sock->sendMessage.  The DEBUG check above is the
     * stricter MTU-compliance assertion; this one is the
     * weaker but mandatory memory-safety floor that must
     * survive a release build. */
    if (data->cursize > NET_MAXMESSAGE)
	Sys_Error("%s: message too big %u", __func__, data->cursize);

    packetLen = NET_HEADERSIZE + data->cursize;

    packetBuffer.length = BigLong(packetLen | NETFLAG_UNRELIABLE);
    packetBuffer.sequence = BigLong(sock->unreliableSendSequence++);
    memcpy(packetBuffer.data, data->data, data->cursize);

    if (sock->landriver->Write(sock->socket, &packetBuffer, packetLen,
			       &sock->addr) == -1)
	return -1;

    packetsSent++;
    return 1;
}


int
Datagram_GetMessage(qsocket_t *sock)
{
    unsigned int length;
    unsigned int flags;
    int ret = 0;
    netadr_t readaddr;
    unsigned int sequence;
    unsigned int count;

    if (!sock->canSend)
	if ((net_time - sock->lastSendTime) > 1.0)
	    ReSendMessage(sock);

    while (1) {
	int actual;
	length = sock->landriver->Read(sock->socket, &packetBuffer,
				       NET_MESSAGESIZE, &readaddr);
	if (length == 0)
	    break;

	if (length == -1) {
	    Con_Printf("Read error\n");
	    return -1;
	}

	if (NET_AddrCompare(&readaddr, &sock->addr) != 0) {
#ifdef DEBUG
	    Con_DPrintf("Forged packet received\n");
	    Con_DPrintf("Expected: %s\n", StrAddr(&sock->addr));
	    Con_DPrintf("Received: %s\n", StrAddr(&readaddr));
#endif
	    continue;
	}

	if (length < NET_HEADERSIZE) {
	    shortPacketCount++;
	    continue;
	}

	/* Stash the wire-read byte count before we overwrite
	 * 'length' with the in-packet announced length below.
	 * Without this the announced length is trusted blindly:
	 * a peer that has cleared the source-address check
	 * (a previously-authenticated client, or anyone on a
	 * spoofable / connectionless transport) can send a
	 * tiny actual datagram whose header announces up to
	 * NETFLAG_LENGTH_MASK (0xFFFF) bytes, and the
	 * SZ_Write / memcpy below then copies up to that many
	 * bytes of stale packetBuffer.data from the previous
	 * recv into net_message or sock->receiveMessage.  In
	 * the DATA path the same overcount accumulates across
	 * fragments into sock->receiveMessage[NET_MAXMESSAGE],
	 * which sock->receiveMessageLength can drive past the
	 * end of that fixed buffer. */
	actual = (int)length;

	length = BigLong(packetBuffer.length);
	flags = length & (~NETFLAG_LENGTH_MASK);
	length &= NETFLAG_LENGTH_MASK;

	/* Announced length must (a) cover at least the header
	 * we already required, and (b) not exceed what we
	 * actually read off the wire.  Drop the packet rather
	 * than try to recover -- it's malformed by definition. */
	if ((int)length < NET_HEADERSIZE || (int)length > actual) {
	    shortPacketCount++;
	    continue;
	}

	if (flags & NETFLAG_CTL)
	    continue;

	sequence = BigLong(packetBuffer.sequence);
	packetsReceived++;

	if (flags & NETFLAG_UNRELIABLE) {
	    if (sequence < sock->unreliableReceiveSequence) {
		Con_DPrintf("Got a stale datagram\n");
		ret = 0;
		break;
	    }
	    if (sequence != sock->unreliableReceiveSequence) {
		count = sequence - sock->unreliableReceiveSequence;
		droppedDatagrams += count;
		Con_DPrintf("Dropped %u datagram(s)\n", count);
	    }
	    sock->unreliableReceiveSequence = sequence + 1;

	    length -= NET_HEADERSIZE;

	    SZ_Clear(&net_message);
	    SZ_Write(&net_message, packetBuffer.data, length);

	    ret = 2;
	    break;
	}

	if (flags & NETFLAG_ACK) {
	    if (sequence != (sock->sendSequence - 1)) {
		Con_DPrintf("Stale ACK received\n");
		continue;
	    }
	    if (sequence == sock->ackSequence) {
		sock->ackSequence++;
		if (sock->ackSequence != sock->sendSequence)
		    Con_DPrintf("ack sequencing error\n");
	    } else {
		Con_DPrintf("Duplicate ACK received\n");
		continue;
	    }
	    sock->sendMessageLength -= sock->mtu;
	    if (sock->sendMessageLength > 0) {
		memmove(sock->sendMessage, sock->sendMessage + sock->mtu,
			sock->sendMessageLength);
		sock->sendNext = true;
	    } else {
		sock->sendMessageLength = 0;
		sock->canSend = true;
	    }
	    continue;
	}

	if (flags & NETFLAG_DATA) {
	    packetBuffer.length = BigLong(NET_HEADERSIZE | NETFLAG_ACK);
	    packetBuffer.sequence = BigLong(sequence);
	    sock->landriver->Write(sock->socket, &packetBuffer, NET_HEADERSIZE,
				   &readaddr);

	    if (sequence != sock->receiveSequence) {
		receivedDuplicateCount++;
		continue;
	    }
	    sock->receiveSequence++;

	    length -= NET_HEADERSIZE;

	    /* receiveMessage is a NET_MAXMESSAGE-sized
	     * accumulator across DATA fragments.  Reject
	     * fragments that would push us off the end --
	     * a peer cycling sequenced DATA packets that
	     * each declare a fragment near the per-packet
	     * cap would otherwise overrun sock->
	     * receiveMessage long before the EOM. */
	    if (sock->receiveMessageLength + length > NET_MAXMESSAGE) {
		Con_DPrintf("Datagram_GetMessage: oversized DATA fragment, "
			    "dropping connection\n");
		return -1;
	    }

	    if (flags & NETFLAG_EOM) {
		SZ_Clear(&net_message);
		SZ_Write(&net_message, sock->receiveMessage,
			 sock->receiveMessageLength);
		SZ_Write(&net_message, packetBuffer.data, length);
		sock->receiveMessageLength = 0;

		ret = 1;
		break;
	    }

	    memcpy(sock->receiveMessage + sock->receiveMessageLength,
		   packetBuffer.data, length);
	    sock->receiveMessageLength += length;
	    continue;
	}
    }

    if (sock->sendNext)
	SendMessageNext(sock);

    return ret;
}

static void
PrintStats(qsocket_t *s)
{
    Con_Printf("canSend = %4u   \n", s->canSend);
    Con_Printf("sendSeq = %4u   ", s->sendSequence);
    Con_Printf("recvSeq = %4u   \n", s->receiveSequence);
    Con_Printf("\n");
}

static void
NET_Stats_f(void)
{
    qsocket_t *s;

    if (Cmd_Argc() == 1) {
	Con_Printf("unreliable messages sent   = %i\n",
		   unreliableMessagesSent);
	Con_Printf("unreliable messages recv   = %i\n",
		   unreliableMessagesReceived);
	Con_Printf("reliable messages sent     = %i\n", messagesSent);
	Con_Printf("reliable messages received = %i\n", messagesReceived);
	Con_Printf("packetsSent                = %i\n", packetsSent);
	Con_Printf("packetsReSent              = %i\n", packetsReSent);
	Con_Printf("packetsReceived            = %i\n", packetsReceived);
	Con_Printf("receivedDuplicateCount     = %i\n",
		   receivedDuplicateCount);
	Con_Printf("shortPacketCount           = %i\n", shortPacketCount);
	Con_Printf("droppedDatagrams           = %i\n", droppedDatagrams);
    } else if (strcmp(Cmd_Argv(1), "*") == 0) {
	for (s = net_activeSockets; s; s = s->next)
	    PrintStats(s);
	for (s = net_freeSockets; s; s = s->next)
	    PrintStats(s);
    } else {
	for (s = net_activeSockets; s; s = s->next)
	    if (strcasecmp(Cmd_Argv(1), s->address) == 0)
		break;
	if (s == NULL)
	    for (s = net_freeSockets; s; s = s->next)
		if (strcasecmp(Cmd_Argv(1), s->address) == 0)
		    break;
	if (s == NULL)
	    return;
	PrintStats(s);
    }
}


struct test_poll_state
{
   qboolean inProgress;
   int pollCount;
   int socket;
   net_landriver_t *driver;
   PollProcedure *procedure;
};


static void
Test_Poll(void *vstate)
{
    netadr_t clientaddr;
    int control;
    int len;
    char *name;
    char *address;
    int colors;
    int frags;
    int connectTime;
    byte playerNumber;
    struct test_poll_state *state = (struct test_poll_state *) vstate;

    while (1) {
	len = state->driver->Read(state->socket, net_message.data,
				  net_message.maxsize, &clientaddr);
	if (len < (int)sizeof(int))
	    break;

	net_message.cursize = len;

	MSG_BeginReading();
	control = MSG_ReadControlHeader();
	if (control == -1)
	    break;
	if ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL)
	    break;
	if ((control & NETFLAG_LENGTH_MASK) != len)
	    break;

	if (MSG_ReadByte() != CCREP_PLAYER_INFO)
	    Sys_Error("Unexpected repsonse to Player Info request");

	playerNumber = MSG_ReadByte();
	name = MSG_ReadString();
	colors = MSG_ReadLong();
	frags = MSG_ReadLong();
	connectTime = MSG_ReadLong();
	address = MSG_ReadString();

	Con_Printf("%s (%d)\n  frags:%3i  colors:%u %u  time:%u\n  %s\n",
		   name, (int)playerNumber, frags, colors >> 4, colors & 0x0f,
		   connectTime / 60, address);
    }

    state->pollCount--;
    if (state->pollCount) {
	SchedulePollProcedure(state->procedure, 0.1);
    } else {
	state->driver->CloseSocket(state->socket);
	state->inProgress = false;
    }
}


static void
Test_f(void)
{
    const char *host;
    int i, n;
    int max = MAX_SCOREBOARD;
    netadr_t sendaddr;
    net_landriver_t *driver = NULL;

    static struct test_poll_state state =
    {
       false,
       0,
       0,
       NULL,
       NULL
    };
    static PollProcedure poll_procedure =
    {
       NULL,
       0.0,
       Test_Poll,
       &state
    };

    if (state.inProgress)
	return;

    host = Cmd_Argv(1);

    if (host && hostCacheCount) {
	for (n = 0; n < hostCacheCount; n++)
	    if (strcasecmp(host, hostcache[n].name) == 0) {
		if (hostcache[n].driver != dgrm_driver)
		    continue;
		driver = hostcache[n].ldriver;
		max = hostcache[n].maxusers;
		sendaddr = hostcache[n].addr;
		break;
	    }
	if (driver)
	    goto JustDoIt;
    }

    for (i = 0; i < net_numlandrivers; i++) {
	driver = &net_landrivers[i];
	if (!driver->initialized)
	    continue;

	/* see if we can resolve the host name */
	if (driver->GetAddrFromName(host, &sendaddr) != -1)
	    break;
    }
    if (!driver)
	return;

  JustDoIt:
    state.socket = driver->OpenSocket(0);
    if (state.socket == -1)
	return;

    state.inProgress = true;
    state.pollCount = 20;
    state.driver = driver;
    state.procedure = &poll_procedure;

    for (n = 0; n < max; n++) {
	SZ_Clear(&net_message);
	/* save space for the header, filled in later */
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREQ_PLAYER_INFO);
	MSG_WriteByte(&net_message, n);
	MSG_WriteControlHeader(&net_message);
	driver->Write(state.socket, net_message.data, net_message.cursize,
		      &sendaddr);
    }
    SZ_Clear(&net_message);
    SchedulePollProcedure(&poll_procedure, 0.1);
}


static void
Test2_Poll(void *vstate)
{
    netadr_t clientaddr;
    int control;
    int len;
    char *name;
    char *value;
    struct test_poll_state *state = (struct test_poll_state *) vstate;

    len = state->driver->Read(state->socket, net_message.data,
			      net_message.maxsize, &clientaddr);
    if (len < (int)sizeof(int))
	goto Reschedule;

    net_message.cursize = len;

    MSG_BeginReading();
    control = MSG_ReadControlHeader();
    if (control == -1)
	goto Error;
    if ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL)
	goto Error;
    if ((control & NETFLAG_LENGTH_MASK) != len)
	goto Error;

    if (MSG_ReadByte() != CCREP_RULE_INFO)
	goto Error;

    name = MSG_ReadString();
    if (!name[0])
	goto Done;
    value = MSG_ReadString();

    Con_Printf("%-16.16s  %-16.16s\n", name, value);

    SZ_Clear(&net_message);
    /* save space for the header, filled in later */
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREQ_RULE_INFO);
    MSG_WriteString(&net_message, name);
    MSG_WriteControlHeader(&net_message);
    state->driver->Write(state->socket, net_message.data, net_message.cursize,
			 &clientaddr);
    SZ_Clear(&net_message);

  Reschedule:
    SchedulePollProcedure(state->procedure, 0.05);
    return;

  Error:
    Con_Printf("Unexpected repsonse to Rule Info request\n");
  Done:
    state->driver->CloseSocket(state->socket);
    state->inProgress = false;
    return;
}

static void
Test2_f(void)
{
   const char *host;
   int i, n;
   netadr_t sendaddr;

   static struct test_poll_state state = {
      false,
      0,
      0,
      NULL,
      NULL
   };
   static PollProcedure poll_procedure = 
   {
      NULL,
      0.0,
      Test2_Poll,
      &state
   };

   if (state.inProgress)
      return;

   host = Cmd_Argv(1);

   if (host && hostCacheCount) {
      for (n = 0; n < hostCacheCount; n++)
         if (strcasecmp(host, hostcache[n].name) == 0) {
            if (hostcache[n].driver != dgrm_driver)
               continue;
            state.driver = hostcache[n].ldriver;
            sendaddr = hostcache[n].addr;
            break;
         }
      if (state.driver)
         goto JustDoIt;
   }

   for (i = 0; i < net_numlandrivers; i++) {
      if (!net_landrivers[i].initialized)
         continue;
      /* see if we can resolve the host name */
      if (net_landrivers[i].GetAddrFromName(host, &sendaddr) != -1) {
         state.driver = &net_landrivers[i];
         break;
      }
   }
   if (!state.driver)
      return;

JustDoIt:
   state.socket = state.driver->OpenSocket(0);
   if (state.socket == -1)
      return;

   state.inProgress = true;
   state.procedure = &poll_procedure;

   SZ_Clear(&net_message);
   /* save space for the header, filled in later */
   MSG_WriteLong(&net_message, 0);
   MSG_WriteByte(&net_message, CCREQ_RULE_INFO);
   MSG_WriteString(&net_message, "");
   MSG_WriteControlHeader(&net_message);
   state.driver->Write(state.socket, net_message.data, net_message.cursize,
         &sendaddr);
   SZ_Clear(&net_message);
   SchedulePollProcedure(&poll_procedure, 0.05);
}


int
Datagram_Init(void)
{
    int i, csock, num_inited;

    dgrm_driver = net_driver;
    Cmd_AddCommand("net_stats", NET_Stats_f);

    if (COM_CheckParm("-nolan"))
	return -1;

    num_inited = 0;
    for (i = 0; i < net_numlandrivers; i++) {
	csock = net_landrivers[i].Init();
	if (csock == -1)
	    continue;
	net_landrivers[i].initialized = true;
	net_landrivers[i].controlSock = csock;
	num_inited++;
    }

    if (num_inited == 0)
	return -1;

    Cmd_AddCommand("ban", NET_Ban_f);
    Cmd_AddCommand("test", Test_f);
    Cmd_AddCommand("test2", Test2_f);

    return 0;
}


void
Datagram_Shutdown(void)
{
    int i;

/**/
/* shutdown the lan drivers */
/**/
    for (i = 0; i < net_numlandrivers; i++) {
	if (net_landrivers[i].initialized) {
	    net_landrivers[i].Shutdown();
	    net_landrivers[i].initialized = false;
	}
    }
}


void
Datagram_Close(qsocket_t *sock)
{
    sock->landriver->CloseSocket(sock->socket);
}


void
Datagram_Listen(qboolean state)
{
    int i;

    for (i = 0; i < net_numlandrivers; i++)
	if (net_landrivers[i].initialized)
	    net_landrivers[i].Listen(state);
}


static qsocket_t *
_Datagram_CheckNewConnections(net_landriver_t *driver)
{
    netadr_t clientaddr;
    netadr_t newaddr;
    netadr_t testAddr;

    int newsock;
    int acceptsock;
    qsocket_t *sock;
    qsocket_t *s;
    int len;
    int command;
    int control;
    int ret;

    acceptsock = driver->CheckNewConnections();
    if (acceptsock == -1)
	return NULL;

    SZ_Clear(&net_message);

    len = driver->Read(acceptsock, net_message.data, net_message.maxsize,
		       &clientaddr);
    if (len < (int)sizeof(int))
	return NULL;
    net_message.cursize = len;

    MSG_BeginReading();
    control = MSG_ReadControlHeader();
    if (control == -1)
	return NULL;
    if ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL)
	return NULL;
    if ((control & NETFLAG_LENGTH_MASK) != len)
	return NULL;

    command = MSG_ReadByte();
    if (command == CCREQ_SERVER_INFO) {
	if (strcmp(MSG_ReadString(), "QUAKE") != 0)
	    return NULL;

	SZ_Clear(&net_message);
	/* save space for the header, filled in later */
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREP_SERVER_INFO);
	driver->GetSocketAddr(acceptsock, &newaddr);
	MSG_WriteString(&net_message, NET_AdrToString(&newaddr));
	MSG_WriteString(&net_message, hostname.string);
	MSG_WriteString(&net_message, sv.name);
	MSG_WriteByte(&net_message, net_activeconnections);
	MSG_WriteByte(&net_message, svs.maxclients);
	MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
	MSG_WriteControlHeader(&net_message);
	driver->Write(acceptsock, net_message.data, net_message.cursize,
		      &clientaddr);
	SZ_Clear(&net_message);
	return NULL;
    }

    if (command == CCREQ_PLAYER_INFO) {
	int playerNumber;
	int activeNumber;
	int clientNumber;
	client_t *client;

	playerNumber = MSG_ReadByte();
	activeNumber = -1;
	for (clientNumber = 0, client = svs.clients;
	     clientNumber < svs.maxclients; clientNumber++, client++) {
	    if (client->active) {
		activeNumber++;
		if (activeNumber == playerNumber)
		    break;
	    }
	}
	if (clientNumber == svs.maxclients)
	    return NULL;

	SZ_Clear(&net_message);
	/* save space for the header, filled in later */
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREP_PLAYER_INFO);
	MSG_WriteByte(&net_message, playerNumber);
	MSG_WriteString(&net_message, client->name);
	MSG_WriteLong(&net_message, client->colors);
	MSG_WriteLong(&net_message, (int)client->edict->v.frags);
	MSG_WriteLong(&net_message,
		      (int)(net_time - client->netconnection->connecttime));
	MSG_WriteString(&net_message, client->netconnection->address);
	MSG_WriteControlHeader(&net_message);
	driver->Write(acceptsock, net_message.data, net_message.cursize,
		      &clientaddr);
	SZ_Clear(&net_message);

	return NULL;
    }

    if (command == CCREQ_RULE_INFO) {
	char *prevCvarName;
	cvar_t *var;

	/* find the search start location */
	prevCvarName = MSG_ReadString();
	var = Cvar_NextServerVar(prevCvarName);
	if (!var)
	    return NULL;

	/* send the response */

	SZ_Clear(&net_message);
	/* save space for the header, filled in later */
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREP_RULE_INFO);
	if (var) {
	    MSG_WriteString(&net_message, var->name);
	    MSG_WriteString(&net_message, var->string);
	}
	MSG_WriteControlHeader(&net_message);
	driver->Write(acceptsock, net_message.data, net_message.cursize,
		      &clientaddr);
	SZ_Clear(&net_message);

	return NULL;
    }

    if (command != CCREQ_CONNECT)
	return NULL;

    if (strcmp(MSG_ReadString(), "QUAKE") != 0)
	return NULL;

    if (MSG_ReadByte() != NET_PROTOCOL_VERSION) {
	SZ_Clear(&net_message);
	/* save space for the header, filled in later */
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREP_REJECT);
	MSG_WriteString(&net_message, "Incompatible version.\n");
	MSG_WriteControlHeader(&net_message);
	driver->Write(acceptsock, net_message.data, net_message.cursize,
		      &clientaddr);
	SZ_Clear(&net_message);
	return NULL;
    }

    /* check for a ban */
    testAddr.ip.l = clientaddr.ip.l;
    if ((testAddr.ip.l & banMask.ip.l) == banAddr.ip.l) {
	SZ_Clear(&net_message);
	/* save space for the header, filled in later */
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREP_REJECT);
	MSG_WriteString(&net_message, "You have been banned.\n");
	MSG_WriteControlHeader(&net_message);
	driver->Write(acceptsock, net_message.data, net_message.cursize,
		      &clientaddr);
	SZ_Clear(&net_message);
	return NULL;
    }

    /* see if this guy is already connected */
    for (s = net_activeSockets; s; s = s->next) {
	if (s->driver != net_driver)
	    continue;
	ret = NET_AddrCompare(&clientaddr, &s->addr);
	if (ret >= 0) {
	    /* is this a duplicate connection reqeust? */
	    if (ret == 0 && net_time - s->connecttime < 2.0) {
		/* yes, so send a duplicate reply */
		SZ_Clear(&net_message);
		/* save space for the header, filled in later */
		MSG_WriteLong(&net_message, 0);
		MSG_WriteByte(&net_message, CCREP_ACCEPT);
		driver->GetSocketAddr(s->socket, &newaddr);
		MSG_WriteLong(&net_message, NET_GetSocketPort(&newaddr));
		MSG_WriteControlHeader(&net_message);
		driver->Write(acceptsock, net_message.data,
			      net_message.cursize, &clientaddr);
		SZ_Clear(&net_message);
		return NULL;
	    }
	    /*
	     * it's somebody coming back in from a crash/disconnect
	     * so close the old qsocket and let their retry get them back in
	     */
	    NET_Close(s);
	    return NULL;
	}
    }

    /* allocate a QSocket */
    sock = NET_NewQSocket();
    if (sock == NULL) {
	/* no room; try to let him know */
	SZ_Clear(&net_message);
	/* save space for the header, filled in later */
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREP_REJECT);
	MSG_WriteString(&net_message, "Server is full.\n");
	MSG_WriteControlHeader(&net_message);
	driver->Write(acceptsock, net_message.data, net_message.cursize,
		      &clientaddr);
	SZ_Clear(&net_message);
	return NULL;
    }
    /* allocate a network socket */
    newsock = driver->OpenSocket(0);
    if (newsock == -1) {
	NET_FreeQSocket(sock);
	return NULL;
    }

    /* everything is allocated, just fill in the details */
    sock->socket = newsock;
    sock->landriver = driver;
    sock->addr = clientaddr;
    strlcpy(sock->address, NET_AdrToString(&clientaddr), sizeof(sock->address));
    sock->mtu = driver->GetDefaultMTU() - NET_HEADERSIZE;

    /* send him back the info about the server connection he has been allocated */
    SZ_Clear(&net_message);
    /* save space for the header, filled in later */
    MSG_WriteLong(&net_message, 0);
    MSG_WriteByte(&net_message, CCREP_ACCEPT);
    driver->GetSocketAddr(newsock, &newaddr);
    MSG_WriteLong(&net_message, NET_GetSocketPort(&newaddr));
    MSG_WriteControlHeader(&net_message);
    driver->Write(acceptsock, net_message.data, net_message.cursize,
		  &clientaddr);
    SZ_Clear(&net_message);

    return sock;
}

qsocket_t *
Datagram_CheckNewConnections(void)
{
    unsigned i;
    net_landriver_t *driver;
    qsocket_t *ret = NULL;

    for (i = 0; i < net_numlandrivers; i++) {
	driver = &net_landrivers[i];
	if (driver->initialized)
	    if ((ret = _Datagram_CheckNewConnections(driver)) != NULL)
		break;
    }

    return ret;
}


static void
_Datagram_SearchForHosts(qboolean xmit, net_landriver_t *driver)
{
    int ret;
    int i, len, hostnum;
    netadr_t readaddr;
    netadr_t myaddr;
    int control;
    hostcache_t *host;

    driver->GetSocketAddr(driver->controlSock, &myaddr);
    if (xmit) {
	SZ_Clear(&net_message);
	/* save space for the header, filled in later */
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREQ_SERVER_INFO);
	MSG_WriteString(&net_message, "QUAKE");
	MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
	MSG_WriteControlHeader(&net_message);
	driver->Broadcast(driver->controlSock, net_message.data,
			  net_message.cursize);
	SZ_Clear(&net_message);
    }

    while ((ret = driver->Read(driver->controlSock, net_message.data,
			       net_message.maxsize, &readaddr)) > 0) {
	if (ret < (int)sizeof(int))
	    continue;
	net_message.cursize = ret;

	/* don't answer our own query */
	if (NET_AddrCompare(&readaddr, &myaddr) >= 0)
	    continue;

	/* is the cache full? */
	if (hostCacheCount == HOSTCACHESIZE)
	    continue;

	MSG_BeginReading();
	control = MSG_ReadControlHeader();
	if (control == -1)
	    continue;
	if ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL)
	    continue;
	if ((control & NETFLAG_LENGTH_MASK) != ret)
	    continue;

	if (MSG_ReadByte() != CCREP_SERVER_INFO)
	    continue;

	driver->GetAddrFromName(MSG_ReadString(), &readaddr);
	/* search the cache for this server */
	for (i = 0, host = hostcache; i < hostCacheCount; i++, host++)
	    if (NET_AddrCompare(&readaddr, &host->addr) == 0)
		break;
	hostnum = i;

	/* is it already there? */
	if (hostnum < hostCacheCount)
	    continue;

	/* add it */
	hostCacheCount++;

	snprintf(host->name, sizeof(host->name), "%s", MSG_ReadString());
	snprintf(host->map, sizeof(host->map), "%s", MSG_ReadString());
	host->users = MSG_ReadByte();
	host->maxusers = MSG_ReadByte();
	if (MSG_ReadByte() != NET_PROTOCOL_VERSION) {
	    strlcpy(host->cname, host->name, sizeof(host->cname));
	    host->cname[14] = 0;
	    strlcpy(host->name, "*", sizeof(host->name));
	    strlcat(host->name, host->cname, sizeof(host->name));
	}
	host->addr = readaddr;
	host->driver = net_driver;
	host->ldriver = driver;
	strlcpy(host->cname, NET_AdrToString(&readaddr), sizeof(host->cname));

	/*
	 * check for a name conflict (FIXME - gross!)
	 */
	for (i = 0; i < hostCacheCount; i++) {
	    if (i == hostnum)
		continue;
	    if (!strcasecmp(host->name, hostcache[i].name)) {
		const int max = sizeof(host->name);
		len = strlen(host->name);
		/*
		 * If there's room to add an extra character and the current
		 * ending character doesn't look like one we might have
		 * just added, add a zero.
		 *
		 * Otherwise, just increment the final character.
		 */
		if (len < max - 1 &&
		    host->name[len - 1] > '0' + HOSTCACHESIZE) {
		    host->name[len] = '0';
		    host->name[len + 1] = 0;
		} else
		    host->name[len - 1]++;
		i = -1; /* reset the loop counter */
	    }
	}
    }
}

void
Datagram_SearchForHosts(qboolean xmit)
{
    int i;
    net_landriver_t *driver;

    for (i = 0; i < net_numlandrivers; i++) {
	if (hostCacheCount == HOSTCACHESIZE)
	    break;
	driver = &net_landrivers[i];
	if (driver->initialized)
	    _Datagram_SearchForHosts(xmit, driver);
    }
}


static qsocket_t *
_Datagram_Connect(const char *host, net_landriver_t *driver)
{
    netadr_t sendaddr;
    netadr_t readaddr;
    qsocket_t *sock;
    int newsock;
    int ret;
    int reps;
    int control;
    const char *reason;

    /* see if we can resolve the host name */
    if (driver->GetAddrFromName(host, &sendaddr) == -1)
	return NULL;

    newsock = driver->OpenSocket(0);
    if (newsock == -1)
	return NULL;

    sock = NET_NewQSocket();
    if (sock == NULL)
	goto ErrorReturn2;

    sock->socket = newsock;
    sock->landriver = driver;
    sock->mtu = driver->GetDefaultMTU() - NET_HEADERSIZE;

    /* send the connection request */
    Con_Printf("trying...\n");
    SCR_UpdateScreen();

    /* This whole function is a busy-wait that
     * runs inside a single retro_run.  In the
     * libretro core net_time / host_time don't
     * advance during a Host_Frame, so the
     * historical "(SetNetTime() - start_time) <
     * 2.5" deadline is unreachable and the inner
     * loop would spin forever.  Replace with an
     * iteration cap.  On modern hardware the
     * driver->Read() call is non-blocking and
     * returns -1 / 0 / >0 promptly, so 250000
     * spins is a defensible analogue of the old
     * 2.5-second timeout (each loop iteration
     * is a syscall + a few branches; very
     * approximately 10us at most).  Datagram_
     * Connect is only invoked from the explicit
     * `connect <ip>` console command, never
     * per-frame in normal SP play. */
    for (reps = 0; reps < 3; reps++) {
	int connect_iters = 0;
	SZ_Clear(&net_message);
	/* save space for the header, filled in later */
	MSG_WriteLong(&net_message, 0);
	MSG_WriteByte(&net_message, CCREQ_CONNECT);
	MSG_WriteString(&net_message, "QUAKE");
	MSG_WriteByte(&net_message, NET_PROTOCOL_VERSION);
	MSG_WriteControlHeader(&net_message);
	driver->Write(newsock, net_message.data, net_message.cursize,
		     &sendaddr);
	SZ_Clear(&net_message);
	do {
	    ret = driver->Read(newsock, net_message.data, net_message.maxsize,
			       &readaddr);
	    /* if we got something, validate it */
	    if (ret > 0) {
		/* is it from the right place? */
		if (NET_AddrCompare(&readaddr, &sendaddr) != 0) {
#ifdef DEBUG
		    Con_Printf("wrong reply address\n");
		    Con_Printf("Expected: %s\n", StrAddr(&sendaddr));
		    Con_Printf("Received: %s\n", StrAddr(&readaddr));
		    SCR_UpdateScreen();
#endif
		    ret = 0;
		    continue;
		}

		if (ret < (int)sizeof(int)) {
		    ret = 0;
		    continue;
		}
		net_message.cursize = ret;

		MSG_BeginReading();
		control = MSG_ReadControlHeader();
		if (control == -1) {
		    ret = 0;
		    continue;
		}
		if ((control & (~NETFLAG_LENGTH_MASK)) != NETFLAG_CTL) {
		    ret = 0;
		    continue;
		}
		if ((control & NETFLAG_LENGTH_MASK) != ret) {
		    ret = 0;
		    continue;
		}
	    }
	} while (ret == 0 && ++connect_iters < 250000);

	if (ret)
	    break;
	Con_Printf("still trying...\n");
	SCR_UpdateScreen();
    }

    if (ret == 0) {
	reason = "No Response";
	goto ErrorReturn;
    }

    if (ret == -1) {
	reason = "Network Error";
	goto ErrorReturn;
    }

    ret = MSG_ReadByte();
    if (ret == CCREP_REJECT) {
	reason = MSG_ReadString();
	goto ErrorReturn;
    }

    if (ret == CCREP_ACCEPT) {
	sock->addr = sendaddr;
	NET_SetSocketPort(&sock->addr, MSG_ReadLong());
    } else {
	reason = "Bad Response";
	goto ErrorReturn;
    }

    driver->GetNameFromAddr(&sendaddr, sock->address);

    Con_Printf("Connection accepted\n");
    sock->lastMessageTime = SetNetTime();
    m_return_onerror = false;

    return sock;

  ErrorReturn:
    Con_Printf("%s\n", reason);
    snprintf(m_return_reason, sizeof(m_return_reason), "%s", reason);
    NET_FreeQSocket(sock);
  ErrorReturn2:
    driver->CloseSocket(newsock);
    if (m_return_onerror) {
	key_dest = key_menu;
	m_state = m_return_state;
	m_return_onerror = false;
    }
    return NULL;
}

qsocket_t *
Datagram_Connect(const char *host)
{
    int i;
    qsocket_t *ret = NULL;
    net_landriver_t *driver;

    for (i = 0; i < net_numlandrivers; i++) {
	driver = &net_landrivers[i];
	if (driver->initialized)
	    if ((ret = _Datagram_Connect(host, driver)) != NULL)
		break;
    }
    return ret;
}
