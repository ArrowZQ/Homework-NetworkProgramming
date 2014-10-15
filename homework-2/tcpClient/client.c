#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 35352
#define MAXBUF 1024

int main(int argc, char *argv[])
{
    char buffer[MAXBUF];
    int receivedBytes = 0;
    int sockfd;
    struct sockaddr_in serveraddr;
    struct hostent *host;

    if (2 != argc)
    {
        printf("Usage: %s <IP address>\n", argv[0]);
        exit(-1);
    }

    if (NULL == (host = gethostbyname(argv[1])))
    {
        perror("gethostbyname error");
        exit(-1);
    }

    if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        perror("create socket failed");
        exit(-1);
    }

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr= ((struct in_addr *)(*host->h_addr_list))->s_addr;

    if (-1 == connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)))
    {
        perror("connect failed");
        exit(-1);
    }
    printf("connect success.\n");

    while(NULL != gets(buffer))
    {
        send(sockfd, buffer, strlen(buffer), 0);
        receivedBytes = recv(sockfd, buffer, MAXBUF, 0);
        if(-1 == receivedBytes)
        {
            perror("receive data error");
            break;
        }
        else if (0 == receivedBytes)
        {
            printf("network disconnected accidentally.\n");
            break;
        }
        buffer[receivedBytes] = '\0';
        printf("reversed data: %s\n", buffer);
    }
    close(sockfd);

    return 0;
}
