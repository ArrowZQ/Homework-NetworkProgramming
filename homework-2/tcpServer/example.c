#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>

#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>//close()
#include<netinet/in.h>//struct sockaddr_in
#include<arpa/inet.h>//inet_ntoa

#define PORT 6001    //端口号
#define BACKLOG 1    //请求队列中的最大连接数

int main()
 {
	 int len;
	 char buf[BUFSIZ];
     int listenfd, connectfd;    //监听套接字描述符与连接套接字描述符
     struct sockaddr_in server, client;    //服务器端和客户端IPV4地址信息
     socklen_t addrlen;

     //使用socket()函数，产生TCP套接字
     if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
     {
         perror("socket() error.\n");
         exit(1);
     }

     int opt = 1;
     //设置套接字选项，使用SO_REUSEADDR允许套接口和一个已经在使用中的地址绑定
     setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
     //初始化server套接字地址结构
     bzero(&server, sizeof(server));
     server.sin_family = AF_INET;
     server.sin_port = htons(PORT);
     server.sin_addr.s_addr = htonl(INADDR_ANY);

     //使用bind()函数，将套接字与指定的协议地址绑定
     if(bind(listenfd, (struct sockaddr *)&server, sizeof(server)) == -1)
     {
         perror("Bind() error\n");
         exit(1);
     }

     //使用listen()函数，等待客户端的连接
     if(listen(listenfd, BACKLOG) == -1)
     {
         perror("listen() error.\n");
         exit(1);
     }

     addrlen = sizeof(client);


     //不断监听客户端的请求
     while(1)
     {
         //接受客户端的连接，客户端的地址信息放在client地址结构中
         if((connectfd = accept(listenfd, (struct sockaddr *)&client, &addrlen)) == -1)
         {
             perror("accept() error\n");
             exit(1);
         }
         //使用inet_ntoa()将网络字节序的二进制IP地址转换成相应的点分十进制IP地址
         printf("You got a connection from client's IP is %s, port is %d\n",
                 inet_ntoa(client.sin_addr), ntohs(client.sin_port));

		 //服务器端对连接的客户端发送一条信息
         send(connectfd, "Welcome\n", 8, 0);
		 while(1)
		 {
			 if((len=recv(connectfd,buf,BUFSIZ,0))>0)
			 {
				 buf[len]='\0';
				 printf("%s\n",buf);
				 if(send(connectfd,buf,len,0)<0)
				 {
					 perror("write\n");
					 return 1;
				 }
			 }
			 else
			 {
				 continue;
			 }
		 }

         //关闭与客户端的连接
         close(connectfd);
     }

     close(listenfd);
     return 0;
 }
