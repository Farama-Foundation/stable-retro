/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2003  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#ifdef NO_SOCKET

int StartServer() { return 0;}
void StopServer() {}
void GetClient() {}
void CloseClient() {}
int HasClient() { return 0;}
int ReadSocket(char * buffer, int len) { return 0;}
int RawReadSocket(char * buffer, int len) { return 0;}
void WriteSocket(char * buffer, int len) {}

void SetsBlock() {}
void SetsNonblock() {}

#else // NO_SOCKET

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "psxcommon.h"
#include "socket.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#endif

static int server_socket = 0;
static int client_socket = 0;

static char tbuf[513];
static int ptr = 0;

#define PORT_NUMBER 12345

int StartServer() {
    struct in_addr localhostaddr;
    struct sockaddr_in localsocketaddr;

#ifdef _WIN32
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
        return -1;
#endif

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
    if (server_socket == INVALID_SOCKET)
        return -1;
#else
    if (server_socket == -1)
        return -1;
#endif

    SetsNonblock();

    memset((void *)&localhostaddr, 0, sizeof(localhostaddr));
    memset(&localsocketaddr, 0, sizeof(struct sockaddr_in));

#ifdef _WIN32
    localhostaddr.S_un.S_addr = htonl(INADDR_ANY);
#else
    localhostaddr.s_addr = htonl(INADDR_ANY);
#endif
    localsocketaddr.sin_family = AF_INET;
    localsocketaddr.sin_addr = localhostaddr;
    localsocketaddr.sin_port = htons(PORT_NUMBER);

    if (bind(server_socket, (struct sockaddr *) &localsocketaddr, sizeof(localsocketaddr)) < 0)
        return -1;

    if (listen(server_socket, 1) != 0)
        return -1;

    return 0;
}

void StopServer() {
#ifdef _WIN32
    shutdown(server_socket, SD_BOTH);
    closesocket(server_socket);
    WSACleanup();
#else
    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);
#endif
}

void GetClient() {
    int new_socket;
    char hello[256];

    new_socket = accept(server_socket, 0, 0);
    
#ifdef _WIN32
    if (new_socket == INVALID_SOCKET)
        return;
#else
    if (new_socket == -1)
        return;
#endif
    if (client_socket)
        CloseClient();
    client_socket = new_socket;

#ifndef _WIN32
    {
        int flags;
        flags = fcntl(client_socket, F_GETFL, 0);
        fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);
    }
#endif

    sprintf(hello, "000 PCSX Version %s - Debug console\r\n", PCSX_VERSION);
    WriteSocket(hello, strlen(hello));
    ptr = 0;
}

void CloseClient() {
    if (client_socket) {
#ifdef _WIN32
        shutdown(client_socket, SD_BOTH);
        closesocket(client_socket);
#else
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
#endif
        client_socket = 0;
    }
}

int HasClient() {
    return client_socket ? 1 : 0;
}

int ReadSocket(char * buffer, int len) {
    int r;
    char * endl;

    if (!client_socket)
        return -1;

    r = recv(client_socket, tbuf + ptr, 512 - ptr, 0);

    if (r == 0) {
        client_socket = 0;
        if (!ptr)
            return 0;
    }
#ifdef _WIN32
    if (r == SOCKET_ERROR)
#else
    if (r == -1)
#endif
    {
        if (ptr == 0)
            return -1;
        r = 0;
    }
    ptr += r;
    tbuf[ptr] = 0;

    endl = strstr(tbuf, "\r\n");

    if (endl) {
        r = endl - tbuf;
        strncpy(buffer, tbuf, r);

        r += 2;
        memmove(tbuf, tbuf + r, 512 - r);
        ptr -= r;
        memset(tbuf + r, 0, 512 - r);
        r -= 2;

    } else {
        r = 0;
    }

    buffer[r] = 0;

    return r;
}

int RawReadSocket(char * buffer, int len) {
    int r = 0;
    int mlen = len < ptr ? len : ptr;

    if (!client_socket)
        return -1;

    if (ptr) {
        memcpy(buffer, tbuf, mlen);
        ptr -= mlen;
        memmove(tbuf, tbuf + mlen, 512 - mlen);
    }

    if (len - mlen)
        r = recv(client_socket, buffer + mlen, len - mlen, 0);

    if (r == 0) {
        client_socket = 0;
        if (!ptr)
            return 0;
    }
#ifdef _WIN32
    if (r == SOCKET_ERROR)
#else
    if (r == -1)
#endif
    {
        if (ptr == 0)
            return -1;
        r = 0;
    }

    r += mlen;

    return r;
}

void WriteSocket(char * buffer, int len) {
    if (!client_socket)
        return;

    send(client_socket, buffer, len, 0);
}

void SetsBlock() {
#ifdef _WIN32
    u_long b = 0;
    ioctlsocket(server_socket, FIONBIO, &b);
#else
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

void SetsNonblock() {
#ifdef _WIN32
    u_long b = 1;
    ioctlsocket(server_socket, FIONBIO, &b);
#else
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);
#endif
}
#endif // NO_SOCKET
