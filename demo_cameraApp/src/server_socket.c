/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "server_socket.h"

#define SOCKET_PORT         10004
#define LISTEN_SOCKET_MAX   10

#define SKT_ERR(fmt, args...)   printf("[ERROR][%s|%d]" fmt, __func__, __LINE__, ##args)
#define SKT_DBG(fmt, args...)   printf("[DEBUG][%s|%d]" fmt, __func__, __LINE__, ##args)
#define SKT_INFO(fmt, args...)   printf("[INFO][%s|%d]" fmt, __func__, __LINE__, ##args)

typedef struct {
    int fd;
    SocketCallbackFunc callback;
    void *args;
} SocketInfo;

static int g_threadRunning = 0;
static pthread_t g_thread;
static SocketInfo g_sockInfo;

static void *DealSocketListen(void *params)
{
    pthread_detach(pthread_self());
    SocketInfo *g_socket = (SocketInfo *)params;

    if (g_socket == NULL || g_socket->fd < 0) {
        SKT_ERR("NULL POINT!\n");
        return NULL;
    }

    while (1) {
        int ret;
        int recvbuf[1024] = {0};
        struct timeval tv = {2, 0};
        fd_set fdset;

        FD_ZERO(&fdset);
        FD_SET(g_socket->fd, &fdset);

        ret = select(g_socket->fd + 1, &fdset, NULL, NULL, &tv);
        if (ret <= 0) {
            continue;
        }

        ret = recv(g_socket->fd, recvbuf, sizeof(recvbuf) - 1, 0);
        if (ret <= 0) {
            SKT_ERR("recv failed! socket may be closed! \n");
            break;
        }
        SKT_INFO("recv msg : %s \n", recvbuf);

        if (g_socket->callback != NULL) {
            g_socket->callback(g_socket->args, (const char *)recvbuf);
        }
    }

    close(g_socket->fd);
    g_socket->fd = -1;

    return NULL;
}

static int ServerSocketCreate(void)
{
    int serverFd = 0;
    struct sockaddr_in s_addr;
    int reuse = 1;
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        SKT_ERR("sokcet failed! errno = %d \n", errno);
        return -1;
    }
    
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&s_addr, 0x00, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(SOCKET_PORT);
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(serverFd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0) {
        SKT_ERR("bind failed! errno = %d \n", errno);
        close(serverFd);
        return -1;
    }
    SKT_DBG("########### bind socket success! ######################## \n\n");

    if (listen(serverFd, LISTEN_SOCKET_MAX) < 0)
    {
        SKT_ERR("listen failed:%d(%s)\n", errno, strerror(errno));
        close(serverFd);
        return -1;
    }
    SKT_DBG("listen success! ######################## \n\n");

    return serverFd;
}

static void *ServerSocketTask(void *args)
{
    int serverFd;
    SocketInfo *info = (SocketInfo *)args;

    pthread_detach(pthread_self());

    serverFd = ServerSocketCreate();
    if (serverFd < 0) {
        SKT_ERR("open socket failed! \n");
        g_threadRunning = 0;
        return NULL;
    }
    SKT_DBG("########### create socket success! ######################## \n\n");
    while (g_threadRunning) {
        int clientFd;
        struct sockaddr_un client_addr;
        int addr_len = sizeof(client_addr);

        memset(&client_addr, 0x00, sizeof(client_addr));

        clientFd = accept(serverFd, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
        if (clientFd >= 0) {
            pthread_t pthread;
            info->fd = clientFd;
            SKT_INFO("new socket connectted! \n");
            if (pthread_create(&pthread, NULL, DealSocketListen, (void *)info) < 0) {
                SKT_ERR("pthread_create DealSocketListen failed! \n");
            }
        }
    }

    close(serverFd);
    serverFd = -1;

    return NULL;
}

int ServerSocketStart(SocketCallbackFunc callback, void *arg)
{
    if (g_threadRunning) {
        SKT_ERR("socket server is running!! \n");
        return 1;
    }

    memset(&g_sockInfo, 0x00, sizeof(g_sockInfo));

    g_sockInfo.fd = -1;
    g_sockInfo.callback = callback;
    g_sockInfo.args = arg;

    g_threadRunning = 1;
    if (pthread_create(&g_thread, NULL, ServerSocketTask, (void *)&g_sockInfo) < 0) {
        SKT_ERR("pthread create ServerSocketTask failed! \n");
        g_threadRunning = 0;
        return -1;
    }

    return 0;
}

void ServerSocketStop(void)
{
    if (g_threadRunning) {
        g_threadRunning = 0;
        g_thread = 0;
    }
}
