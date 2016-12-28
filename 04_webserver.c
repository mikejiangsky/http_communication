#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/************************************************************************
函数名称：	int main(int argc, char *argv[])
函数功能：	通过进程创建webserver
函数参数：	int argc, char *argv[]
函数返回：	无
************************************************************************/
int main(int argc, char *argv[])
{
	unsigned short port = 8000;   //设置默认端口号
	if(argc > 1)
	{
		port = atoi(argv[1]);	//将参数2赋值给端口号变量
	}
	
	//创建TCP套接字
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if( sockfd < 0)
	{
		perror("socket");	
		exit(-1);
	}
	
	//服务器套接字地址变量赋值
	struct sockaddr_in my_addr;
	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = AF_INET;   //IPV4族
	my_addr.sin_port   = htons(port); //将端口号转换成网络字节序
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY); //本机IP地址
	
	//绑定TCP套接字
	if( bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) != 0)
	{
		perror("bind");
		close(sockfd);		
		exit(-1);
	}
	
	//监听
	if( listen(sockfd, 10) != 0)
	{
		perror("listen");
		close(sockfd);		
		exit(-1);
	}

	printf("Listenning at port=%d\n",port);   //打印端口号信息
	printf("Usage: http://127.0.0.1:%d/html/index.html\n", port);
	
	while(1)
	{
		char cli_ip[INET_ADDRSTRLEN] = {0};  //存放客户端点分十进制IP地址
		struct sockaddr_in client_addr;
		socklen_t cliaddr_len = sizeof(client_addr);
		
		//等待客户端连接
		int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &cliaddr_len);
		printf("connfd=%d\n",connfd); //打印已连接套接字
		if(connfd > 0)
		{
			if(fork() == 0)  //创建进程并判断返回值
			{
				close(sockfd);
				//子进程执行
				int  fd = 0;
				int  len = 0;
				char buf[1024] = "";
				char filename[50] = "";
				
				//将网络字节序转换成点分十进制形式存放在cli_ip中
				inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);
				printf("connected form %s\n\r", cli_ip);   //打印点分十进制形式的客户端IP地址
				recv(connfd, buf, sizeof(buf), 0);   //接收客户端发送的请求内容
				sscanf(buf, "GET /%[^ ]", filename);   //解析客户端发送请求字符串
				printf("filename=*%s*\n", filename);
				
				fd = open(filename, O_RDONLY);   //以只读方式打开文件
				if( fd < 0)   //如果打开文件失败
				{
					//HTTP失败头部
					char err[]=	"HTTP/1.1 404 Not Found\r\n"
								"Content-Type: text/html\r\n"
								"\r\n"	
								"<HTML><BODY>File not found</BODY></HTML>";
								
					perror("open error");					
					send(connfd, err, strlen(err), 0);
					close(connfd);  //关闭已连接套接字
					exit(0);	//子进程退出
				}
				
				//打开文件成功后
				//接收成功时返回的头部
				char head[]="HTTP/1.1 200 OK\r\n"
							"Content-Type: text/html\r\n"
							"\r\n";
				send(connfd, head, strlen(head), 0);  //发送HTTP请求成功头部
				
				while( (len = read(fd, buf, sizeof(buf))) > 0)   //循环读取文件内容
				{
					send(connfd, buf, len, 0);       //将读得的数据发送给客户端
				}
				
				close(fd);   //成功后关闭文件
				close(connfd);   //关闭已连接套接字
				exit(0);	 //子进程退出
			}
		}	
		
		close(connfd);   //父进程关闭连接套接字
	}
	close(sockfd);
	printf("exit main!\n");
	
	return 0;
}
