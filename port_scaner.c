#include<stdio.h>
#include<WinSock2.h>
#pragma comment(lib,"ws2_32")
#define START 79 //起始端口
#define END 1025 //终止端口
int main(int argc,char *argv[])
{
	if(argc!=2)
	{
		printf("Usage:scanport<IP>\n");
		return 0;
	}
	int i;
	WSADATA ws;
	SOCKET sockfd;
	struct sockaddr_in their_addr;
	WSAStartup(MAKEWORD(2,2),&ws);
	their_addr.sin_family=AF_INET;
	their_addr.sin_addr.S_un.S_addr=inet_addr(argv[1]);
	//根据命令行参数确定扫描IP
	for(i=START;i<=END;i++)
	{
		//循环建立socket后连接
		sockfd=socket(AF_INET,SOCK_STREAM,0);
		their_addr.sin_port=htons(i);
		printf("正在扫描端口：%d\n",i);
		if(connect(sockfd,(struct sockaddr*)&their_addr,sizeof(struct sockaddr))==SOCKET_ERROR)
		{
			//如果连接失败，则直接进行下一个端口的扫描
			continue;
		}
		//否则认为此端口开放
		printf("\n\t端口 %d 开放！\n\n",i);
	}
	closesocket(sockfd);
	WSACleanup();
	return 0;
}
