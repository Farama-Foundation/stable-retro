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

#include "net.h"
#include "net_dgrm.h"
#include "net_loop.h"
#include "net_udp.h"

net_driver_t net_drivers[] = {
   {
      "Loopback",                      /* name */
      false,                           /* initialized */
      Loop_Init,                       /* Init */
      Loop_Listen,                     /* Listen */
      Loop_SearchForHosts,             /* SearchForHosts */
      Loop_Connect,                    /* Connect */
      Loop_CheckNewConnections,        /* CheckNewConnections */
      Loop_GetMessage,                 /* QGetMessage */
      Loop_SendMessage,                /* QSendMessage */
      Loop_SendUnreliableMessage,      /* SendUnreliableMessage */
      Loop_CanSendMessage,             /* CanSendMessage */
      Loop_CanSendUnreliableMessage,   /* CanSendUnreliableMessage */
      Loop_Close,                      /* Close */
      Loop_Shutdown,                   /* Shutdown */
      0                                /* controlSock */
   },
   {
      "Datagram",                      /* name */
      false,                           /* initialized */
      Datagram_Init,                   /* Init */
      Datagram_Listen,                 /* Listen */
      Datagram_SearchForHosts,         /* SearchForHosts */
      Datagram_Connect,                /* Connect */
      Datagram_CheckNewConnections,    /* CheckNewConnections */
      Datagram_GetMessage,             /* QGetMessage */
      Datagram_SendMessage,            /* QSendMessage */
      Datagram_SendUnreliableMessage,  /* SendUnreliableMessage */
      Datagram_CanSendMessage,         /* CanSendMessage */
      Datagram_CanSendUnreliableMessage,  /* CanSendUnreliableMessage */
      Datagram_Close,                     /* Close */
      Datagram_Shutdown,                  /* Shutdown */
      0                                   /* controlSock */
   }
};

int net_numdrivers = 2;

net_landriver_t net_landrivers[] = {
   {
      "UDP",                                /* name */
      false,                                /* initialized */
      0,                                    /* controlSock */
      UDP_Init,                             /* Init */
      UDP_Shutdown,                         /* Shutdown */
      UDP_Listen,                           /* Listen */
      UDP_OpenSocket,                       /* OpenSocket */
      UDP_CloseSocket,                      /* CloseSocket */
      UDP_CheckNewConnections,              /* CheckNewConnections */
      UDP_Read,                             /* Read */
      UDP_Write,                            /* Write */
      UDP_Broadcast,                        /* Broadcast */
      UDP_GetSocketAddr,                    /* GetSocketAddr */
      UDP_GetNameFromAddr,                  /* GetNameFromAddr */
      UDP_GetAddrFromName,                  /* GetAddrFromName */
      UDP_GetDefaultMTU                     /* GetDefaultMTU */
   }
};

int net_numlandrivers = 1;
