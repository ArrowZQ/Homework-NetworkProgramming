#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define PORT 35352
#define BACKLOG 10
#define MAXBUF 1024

pthread_key_t   key_headtail;
pthread_once_t key_once;
struct ARG {
    int connfd;
    struct sockaddr_in client;
};

struct userInput {
    char *input;
    struct userInput *next;
};

struct headTail {
    struct userInput *head;
    struct userInput *tail;
};

int insert(struct headTail *headtail, char *buffer)
{
    struct userInput **root = &headtail->head;
    struct userInput **current = &headtail->tail;
    struct userInput *newone = NULL;
    newone = malloc(sizeof(struct userInput));
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
        *current = newone;
    }
    else
    {
        (*current)->next = newone;
        *current = newone;
    }
    return 0;

}

void reverse(char *str, int len)
{
    char *strLeft = str;
    char *strRight = str + len - 1;

    while(strRight > strLeft)
    {
        char temp = *strLeft;
        *strLeft = *strRight;
        *strRight = temp;
        --strRight;
        ++strLeft;
    }
}

void printInput()
{
    struct headTail *headtail = pthread_getspecific(key_headtail);
    struct userInput *root = headtail->head;
    struct userInput *tmp = NULL;
    while(root != NULL)
    {
        tmp = root;
        printf("%s\n", root->input);
        root = root->next;
        free(tmp->input);
        free(tmp);
    }
}

void process_cli(int connectfd, struct sockaddr_in client)
{
    struct headTail *headtail = malloc(sizeof(struct headTail));
    pthread_setspecific(key_headtail, headtail);
    char buffer[MAXBUF];
    int receivedBytes = -2;

    printf("You got a connection from client,IP is %s, PORT is %d\n",
                 inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    /*receive the name of client*/
    receivedBytes = recv(connectfd, buffer, MAXBUF, 0);
    if(-1 == receivedBytes)
    {
        perror("receive data of name of client error");
    }
    else if (0 == receivedBytes)
    {
        printf("network disconnected accidentally.\n");
    }
    else
    {
        buffer[receivedBytes] = '\0';
        printf("The name of client is %s.\n", buffer);
        if (insert(headtail, buffer))
        {
            printf("insert name failed.\n");
        }
        else
        {
            /*recieve data*/
            while(1)
            {
                receivedBytes = recv(connectfd, buffer, MAXBUF, 0);
                if(-1 == receivedBytes)
                {
                    perror("receive data error");
                    break;
                }
                else if (0 == receivedBytes)
                {
                    /*printf("network disconnected accidentally.\n");*/
                    break;
                }

                buffer[receivedBytes] = '\0';

                if (insert(headtail, buffer))
                {
                    printf("insert failed.\n");
                    break;
                }

                reverse(buffer, receivedBytes);
                /*send data*/
                send(connectfd, buffer, strlen(buffer), 0);

            }
        }

    }
    printInput();
    close(connectfd);
    printf("[info]:close:%s.\n", inet_ntoa(client.sin_addr));
}

void processCli_destructor(void *ptr)
{
    free(ptr);
}

void processCli_once(void)
{
    pthread_key_create(&key_headtail, processCli_destructor);
}

void *start_routine(void *arg)
{
    pthread_once(&key_once, processCli_once);
    struct ARG *info;
    info = (struct ARG *)arg;
    process_cli(info->connfd, info->client);
    free(arg);
    pthread_exit(NULL);
}

int main(void)
{
    pthread_t tid;
    struct ARG *arg;
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
        arg = malloc(sizeof(struct ARG));
        arg->connfd = connectfd;
        memcpy(&arg->client, &clientaddr, sizeof(clientaddr));
        if (pthread_create(&tid, NULL, start_routine, arg))
        {
            printf("Create pthread error.\n");
        }
    }

    close(listenfd);
    return 0;
}
