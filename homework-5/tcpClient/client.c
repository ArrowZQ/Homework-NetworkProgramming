#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#define PORT 35352

void fun(int sig)
{
    printf("Dealing with interrupt in signal fuction.\n");
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in serveraddr;
    struct hostent *host;
    struct sigaction act;

    act.sa_handler = fun;
    act.sa_flags = SA_RESTART ;


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

    sigaction(SIGINT, &act, NULL);

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr= ((struct in_addr *)(*host->h_addr_list))->s_addr;

    if (-1 == connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)))
    {
        if (errno == EINTR)
        {
            printf("interupted.\n");
            exit(-1);
            //sleep(1);
            //continue;
        }
        else
        {
            perror("connect failed");
            exit(-1);
        }
    }

    printf("connect success.");
    close(sockfd);

    return 0;
}
