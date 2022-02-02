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

#include "client_socket.h"

#define USER_ALARM  0

#define SOCKET_IP       "127.0.0.1"
#define SOCKET_PORT     10004
#define CMD_DEAL_TIME   1500000

#define CLS_ERR(fmt, args...)   printf("[CLIENT_SEND_ERROR][%s|%d]" fmt, __func__, __LINE__, ##args)
#define CLS_DBG(fmt, args...)   printf("[CLIENT_SEND_DEBUG][%s|%d]" fmt, __func__, __LINE__, ##args)
#define CLS_INFO(fmt, args...)   printf("[CLIENT_SEND_INFO][%s|%d]" fmt, __func__, __LINE__, ##args)

static void ClientSetMsgInvalid(int valid);

static int ClientSendMsg(const char *msg)
{
    int sockfd = -1;
    struct sockaddr_in c_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        CLS_ERR("socket failed! errno : %d\n", errno);
        return -1;
    }

    memset(&c_addr, 0x00, sizeof(c_addr));
    c_addr.sin_family = AF_INET;
    c_addr.sin_port = htons(SOCKET_PORT);
    c_addr.sin_addr.s_addr = inet_addr(SOCKET_IP);

    if (connect(sockfd, (struct sockaddr *)&c_addr, sizeof(c_addr)) < 0) {
        CLS_ERR("connect failed!!! errno : %d \n", errno);
        close(sockfd);
        return -1;
    }
    CLS_INFO("connect ok! send %s \n\n", msg);

    if (send(sockfd, msg, strlen(msg), 0) < 0) {
        CLS_ERR("send failed!! errno = %d \n", errno);
    }

    close(sockfd);
#if (USER_ALARM == 0)
    usleep(CMD_DEAL_TIME);       // wait cmd deal!
#endif
    ClientSetMsgInvalid(0);

    return 0;
}

static void *SendTask(void *arg)
{
    pthread_detach(pthread_self());
    if (ClientSendMsg((const char *)arg) < 0) {
        CLS_ERR("Client send msg(%s) failed! \n", (char *)arg);
    }

    return NULL;
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int msgInvalid = 0;

static int ClientGetMsgInvalid(void)
{
    int valid;

    pthread_mutex_lock(&mutex);
    valid = msgInvalid;
    pthread_mutex_unlock(&mutex);

    return valid;
}

static void ClientSetMsgInvalid(int valid)
{
    pthread_mutex_lock(&mutex);
    msgInvalid = valid;
    pthread_mutex_unlock(&mutex);
}

#if (USER_ALARM == 1)
static void TimerHandle(int sig)
{
    printf("######### alarm TimerHandle ########### \n\n");
    ClientSetMsgInvalid(0);
}
#endif

int ClientSend(const char *msg)
{
    pthread_t threadId;

    if (ClientGetMsgInvalid() != 0) {
        CLS_ERR("msg has been sending! \n");
        return 1;
    }

    ClientSetMsgInvalid(1);

    if (pthread_create(&threadId, NULL, SendTask, (void *)msg) < 0) {
        CLS_ERR("pthread_create SendTask failed! \n");
        return -1;
    }
#if (USER_ALARM == 1)
    signal(SIGALRM, TimerHandle);
    alarm(2);                       // alarm time 2s
#endif
    return 0;
}
