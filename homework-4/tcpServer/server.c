#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PORT 35352
#define BACKLOG 10
#define MAXBUF 1024

struct clientInput {
    char *input;
    struct clientInput *next;
};

struct clientInfo {
    int socketfd;
    char *ip;
    uint16_t port;
    struct clientInput *root;
    struct clientInput *tail;
    struct clientInfo *next;
};

int addClient(struct clientInfo **root, struct clientInfo **tail, char *ip, uint16_t port, int connectedfd){
    struct clientInfo *newClientInfo = malloc(sizeof(struct clientInfo));
    char *tmpIP = malloc(strlen(ip) + 1);
    if (newClientInfo == NULL || tmpIP == NULL)
    {
        printf("malloc error,did not add user.\n");
        return -1;
    }
    strcpy(tmpIP, ip);
    newClientInfo->ip = tmpIP;
    newClientInfo->port = port;
    newClientInfo->socketfd = connectedfd;
    newClientInfo->root = NULL;
    newClientInfo->tail = NULL;
    newClientInfo->next = NULL;

    if (NULL == *root)
    {
        *root = newClientInfo;
        *tail = newClientInfo;
    }
    else
    {
        (*tail)->next = newClientInfo;
        *tail = newClientInfo;
    }
    return 0;
}
void printAnddeleteClient(struct clientInfo *deleted, struct clientInfo **previous)
{
    if (deleted == *previous)
    {
        *previous = NULL;
    }
    else
    {
        (*previous)->next = deleted->next;
    }

    printf("========%s,%d said:========\n", deleted->ip, deleted->port);
    struct clientInput *tmp = NULL;
    struct clientInput *root = deleted->root;
    while(root != NULL)
    {
        tmp = root;
        printf("%s\n", root->input);
        root = root->next;
        free(tmp->input);
        free(tmp);
    }
    printf("===========================\n");
    free(deleted->ip);
    free(deleted);

}

int addInput(struct clientInfo *client, char *buffer)
{
    struct clientInput **root = &client->root;
    struct clientInput **tail = &client->tail;
    struct clientInput *newone = NULL;
    newone = malloc(sizeof(struct clientInput));
    newone->input = malloc(sizeof(strlen(buffer) + 1));
    if (NULL == newone || NULL == newone->input)
    {
        return 1;
    }
    memcpy(newone->input, buffer, strlen(buffer) + 1);
    newone->next = NULL;
    if (NULL == *root)
    {
        *root = newone;
        *tail = newone;
    }
    else
    {
        (*tail)->next = newone;
        *tail = newone;
    }
    return 0;

}

int main(void)
{
    char buffer[MAXBUF];
    int receivedBytes = -2;
    socklen_t clientaddrLen;
    int listenfd, connectfd, sockfd;
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    int nready;
    fd_set rset, allset;
    int maxfd;
    struct clientInfo *clientInfoRoot = NULL;
    struct clientInfo *clientInfoTail = NULL;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == listenfd)
    {
        perror("Create listen socket failed");
        exit(-1);
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (-1 == bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)))
    {
        perror("Bind error");
        exit(-1);
    }

    if (-1 == listen(listenfd, BACKLOG))
    {
        perror("listen error");
        exit(-1);
    }

    maxfd = listenfd;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    clientaddrLen = sizeof(clientaddr);
    printf("Server started. \n");
    while(1)
    {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (-1 == nready)
        {
            perror("Select error");
            exit(-1);
        }
        if (FD_ISSET(listenfd, &rset))
        {
            if (-1 == (connectfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientaddrLen)))
            {
                perror("accept error");
                exit(-1);
            }

            char *clientip = inet_ntoa(clientaddr.sin_addr);
            uint16_t clientport = ntohs(clientaddr.sin_port);
            if(0 == addClient(&clientInfoRoot, &clientInfoTail, clientip, clientport, connectfd))
            {
                printf("You got a connection from client,IP is %s, PORT is %d\n",
                 clientip, clientport);
                FD_SET(connectfd, &allset);
                if (connectfd > maxfd)
                {
                    maxfd = connectfd;
                }
            }
            if (--nready <= 0)
            {
                continue;
            }
        }

        struct clientInfo *tmpClientInfo = clientInfoRoot;
        struct clientInfo *previous = NULL;
        while(tmpClientInfo != NULL)
        {
            sockfd = tmpClientInfo->socketfd;

            if (FD_ISSET(sockfd, &rset))
            {
                receivedBytes = recv(sockfd, buffer, MAXBUF, 0);
                if(-1 == receivedBytes || 0 == receivedBytes)
                {
                    /*perror("receive data error");*/
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    if (NULL == previous)
                    {
                        printAnddeleteClient(tmpClientInfo, &clientInfoRoot);
                    }
                    else
                    {
                        printAnddeleteClient(tmpClientInfo, &previous);
                    }

                }
                else
                {
                    buffer[receivedBytes] = '\0';
                    printf("%s,%d:%s\n", tmpClientInfo->ip, tmpClientInfo->port, buffer);
                    addInput(tmpClientInfo, buffer);
                    send(sockfd, buffer, strlen(buffer), 0);
                }

                if (--nready <= 0)
                {
                    break;
                }
            }

            previous = tmpClientInfo;
            tmpClientInfo = tmpClientInfo->next;
        }
    }
    close(listenfd);
    return 0;
}
