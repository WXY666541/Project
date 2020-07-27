#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>

char *file[]={"main.c","main.cpp"};
struct Head
{
	int language;
	int file_size;
};

//1.与服务器建立连接
int StartLink()
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
	{
		return -1;
	}

	struct sockaddr_in ser;
	memset(&ser,0,sizeof(ser));
	ser.sin_family = AF_INET;
	ser.sin_port = htons(6000);
	ser.sin_addr.s_addr = inet_addr("127.0.0.1");

	int res = connect(sockfd,(struct sockaddr*)&ser,sizeof(ser));
	if(res == -1)
	{
		return -1;
	}
	return sockfd;
}

//2.用户选择语言
int ChoiceLanguage()
{
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	printf("~~~~~~~~~ 1   c 语言   ~~~~~~~~~~~~\n");
	printf("~~~~~~~~~ 2   c++     ~~~~~~~~~~~~\n");
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	printf("请输入您编写语言对应的数字: ");
	int language = 0;
	scanf("%d",&language);
	return language;
}

//3.用户输入代码,通过进程替换的方式打开系统vim，用户输入代码
void WriteCoding(int flag,int language)
{
	//flag标志用户选择创建新文件还是打开上一次的文件继续编辑
	//创建新文件需要先删除原有的文件，再vim打开新的，打开上一次的直接vim打开即可
	if(flag == 2)
	{
		unlink(file[language -1]);//删除文件
	}

	pid_t pid = fork();
	assert(pid != -1);
	if(pid == 0)
	{
		//子进程替换vim,创建一个文件
		execl("/usr/bin/vim","/usr/bin/vim",file[language-1],(char*)0);
		printf("exec vim error\n");
		exit(0);
	}
	else
	{
		wait(NULL);
	}
}

//4.发送数据
int  SendData(int sockfd,int language)
{
	//获取文件属性
	struct stat st;
	stat(file[language-1],&st);

	//用户打开文件没有输入数据的特殊情况
	if(st.st_size == 0)
	{
		int empty = 1;
		return empty;
	}

	//发送文件头
	struct Head head;
	head.language = language;
	head.file_size = st.st_size;

	send(sockfd,&head,sizeof(head),0);

	//发送文件内容
	int fd = open(file[language-1],O_RDONLY);
	while(1)
	{
		char buff[128] = {0};
		int n = read(fd,buff,127);
		if(n <= 0)
		{
			break;
		}
		send(sockfd,buff,n,0);
	}
	close(fd);
}

//5.读取服务器反馈信息
void RecvData(int sockfd)
{
	//为了防止粘包问题，先接收文件大小
	int size;
	recv(sockfd,&size,4,0);

	//接收文件内容
	int num = 0;
	printf("*************  编译运行结果为 ***************\n");
	while(1)
	{
		int x = size - num >127?127:size-num;
		char buff[128] = {0};
		int n = recv(sockfd,buff,x,0);
		if(n <= 0)
		{
			close(sockfd);
			exit(0);
		}
		printf("%s\n",buff);
		num += n;
		if(num >= size)
		{
			break;
		}
	}
	printf("**********************************************\n");
}

//6.用户选择下一次操作
int PrintTag()
{
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	printf("~~~~~~~ 1  修改代码     ~~~~~~~\n");
	printf("~~~~~~~ 2  编写下一个程序~~~~~~~\n");
	printf("~~~~~~~ 3  退出程序     ~~~~~~~\n");
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	printf("please input number: ");
	int flag = 0;
	scanf("%d",&flag);
	return flag;
}

int main()
{
	//1、服务器建立链接
	int sockfd = StartLink();
	assert(sockfd != -1);
	
	//2、用户选择语言
	int language = ChoiceLanguage();

	//第一次使用和编写下一个代码逻辑相同，所有初始化falg = 2
	int flag = 2;

	while(1)
	{
		//3、用户输入代码
		WriteCoding(flag,language);
		//4、将选择的语言和代码发送给服务器
		int empty = 0;
		empty = SendData(sockfd,language);
		if(!empty)//表示文件不为空
		{
			//5、获取服务器反馈的结果
			RecvData(sockfd);
		}
		//6、用户选择下一次操作
		flag = PrintTag();
		if(flag == 3)
		{
			break;
		}
	}
	close(sockfd);
}

