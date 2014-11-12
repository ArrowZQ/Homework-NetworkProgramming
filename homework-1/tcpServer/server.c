#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PORT 35351
#define BACKLOG 1

int main(void)
{
    socklen_t clientaddrLen;
    int listenfd, connectfd;
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;

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

    clientaddrLen = sizeof(clientaddr);
    printf("Server started. \n");
    while(1)
    {
        if (-1 == (connectfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientaddrLen)))
        {
            perror("accept error");
            exit(-1);
        }
        printf("You got a connection from client,IP is %s, PORT is %d\n",
                 inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        close(connectfd);
    }

    close(listenfd);
    return 0;
}
