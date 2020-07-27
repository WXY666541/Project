#define _GNC_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<errno.h>

#define MAXEVENTS 100
char *file[] = {"main.c","main.cpp"};//创建的文件种类
char* build[] = {"/usr/bin/gcc","/usr/bin/g++"};//系统自带的编译器存储路径
char *carry[] = {"./a.out","./a.out"};//代码运行指令

//定义一个文件头，每次数据交互的时候先发送，解决粘包问题
struct Head
{
	int language;
	int file_size;
};

//一、创建一个监听套接字
int CreateSocket()
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
	{
		return -1;
	}

	struct sockaddr_in ser;
	memset(&ser,0,sizeof(ser));
	ser.sin_family = AF_INET;//地址簇
	ser.sin_port = htons(6000);//主机字节序转网络字节序
	ser.sin_addr.s_addr = inet_addr("127.0.0.1");//点分十进制转转网络字节序标准IPV4

	int res = bind(sockfd,(struct sockaddr*)&ser,sizeof(ser));
	if(res == -1)
	{
		return -1;
	}

	res = listen(sockfd,5);//5是为内核维护的完成三次握手的连接
	if(res == -1)
	{
		return -1;
	}
	return sockfd;
}

//1.获取一个新的链接
void GetNewClient(int sockfd,int epfd)
{
	struct sockaddr_in cli;
	socklen_t len = sizeof(cli);//保存连接的客户端信息

	int fd = accept(sockfd,(struct sockaddr*)&cli,&len);
	if(fd < 0)
	{
		return;
	}
	printf("客户端%d已连接\n",fd);

	//设置新的链接关注的事件并将其添加到内核事件表中
	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
	epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);

	int flag = fcntl(fd,F_GETFL);//获得文件描述符状态
	flag = flag | O_NONBLOCK;//将状态设置为非阻塞
	fcntl(fd,F_SETFL,flag);//将非阻塞状态设置给文件描述符

}

//3.1接收客户端数据，返回用户传递的语言类型
int RecvCoding(int fd)
{
	//接收协议头。根据语言类型创建对应文件
	struct Head head;
	recv(fd,&head,sizeof(head),0);

	int filefd = open(file[head.language-1],O_WRONLY|O_TRUNC|O_CREAT,0664);
	int size = 0;

	//接收代码
	while(1)
	{
		int num = head.file_size -size >127?127: head.file_size -size;
		char buff[127] = {0};
		int n = recv(fd,buff,num,0);
		if(n == 0)
		{
			break;
		}
		if(n == -1)
		{
			//表示缓冲区中没有数据可读，唤醒sockfd进行下一次读操作，如果不设置文件描述符一直等着数据到来
			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				printf("ser read over\n");
				break;
			}
		}
		size+=n;
		write(filefd,buff,n);//将接收到的数据存储到可执行文件中
		if(size >= head.file_size)
		{
			break;
		}
	}
	close(filefd);
	return head.language;//返回语言类型，根据语言类型进行语言编译
}

//3.3执行代码，替换程a.out程序，将执行结果保存在文件中
void Carry(int language)
{
	pid_t pid = fork();
	assert(pid != -1);
	if(pid == 0)
	{
		int fd = open("./result.txt",O_WRONLY|O_TRUNC|O_CREAT,0664);
		close(1);
		close(2);
		dup(fd);
		dup(fd);
		execl(carry[language-1],carry[language-1],(char*)0);
		write(fd,"carry error",11);
		exit(0);
	}
	else
	{
		wait(NULL);
	}
}

//3.4发送结果
void SendResult(int fd,int flag)
{
	char* file = "./result.txt";
	if(flag)
	{
		file = "./build_error.txt";
	}
	struct stat st;
	stat(file,&st);

	//发送文件大小
	send(fd,(int*)&st.st_size,4,0);

	//发送服务器反馈内容
	int filefd = open(file,O_RDONLY);
	while(1)
	{
		char buff[128] = {0};
		int n = read(filefd,buff,127);
		if(n <= 0)
		{
			break;
		}
		send(fd,buff,n,0);
	}
	close(filefd);
}
//3.2编译代码，替换为系统自带的编译器进行编译
int BuildCoding(int language)
{
	struct stat st;//定义存储文件信息的结构体
	pid_t pid = fork();
	assert(pid != -1);
	
	//子进程编写程序
	if(pid == 0)
	{
		int fd = open("./build_error.txt",O_CREAT | O_WRONLY|O_TRUNC,0664);
		close(1);//关闭标准输入
		close(2);//关闭标准错误输入
		dup(fd);//将文件描述副重定位
		dup(fd);
		//进程替换编译文件
		execl(build[language-1],build[language-1],file[language-1],(char*)0);
		write(fd,"build error",11);//替换失败的处理
		exit(0);
	}
	else
	{
		wait(NULL);//阻塞等待子进程的结束
		stat("./build_error.txt",&st);//将build_error文件大小放到st结构中
	}
	return st.st_size;//返回错误文件大小，0表示编译成功，>0表示编译失败
}

//3.处理客户端的数据
void DealClientData(int fd)
{
	//3.1接收客户端的数据，将代码存储到本地文件中
	int language = RecvCoding(fd);

	//3.2编译代码，将编译结果存储到编译错误文件中
	int flag = BuildCoding(language);
	if(flag == 0)
	{
		//3.3执行代码，结果存储到文件中
		Carry(language);
		//3.4发送执行结果
		SendResult(fd,flag);
	}
	else
	{
		//发送编译失败执行结果
		SendResult(fd,flag);
	}
}


//五、处理就绪事件
void DealFinishEvents(int sockfd,int epfd,struct epoll_event *events,int num)
{
	int i = 0;
	for(;i<num;i++)
	{
		//获取就绪文件描述符的值
		int fd = events[i].data.fd;

		//1.有新的客户端链接
		if(fd == sockfd)
		{
			GetNewClient(sockfd,epfd);
		}
		else
		{
			//2.断开链接的事件
			if(events[i].events & EPOLLRDHUP)
			{       
				close(fd);
				epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
			}
			//3.处理客户端数据
			else
			{
				DealClientData(fd);
			}
		}
	}
}


int main()
{
	//一、创建一个套接字
	int sockfd = CreateSocket();
	assert(sockfd != -1);

	//二、创建一个内核事件表
	int epfd = epoll_create(5);
	assert(epfd != -1);

	//三、将sockfd添加到内核事件表中
	struct epoll_event event;
	event.data.fd = sockfd;
	event.events = EPOLLIN;//用户关注的事件类型

	epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&event);
	
	//四、监听内核事件表上的所有文件描述符
	while(1)
	{
		struct epoll_event events[MAXEVENTS];
		int n = epoll_wait(epfd,events,MAXEVENTS-1,-1);
		if(n <= 0)
		{
			printf("epoll_wait error\n");
			continue;
		}

		//五、处理就绪事件
		DealFinishEvents(sockfd,epfd,events,n);
	}
}


