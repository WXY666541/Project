# include <stdio.h>
# include <stdlib.h>
# include <assert.h>
# include <string.h>
# include <unistd.h>
# include <error.h>
# include <fcntl.h>
# include <pwd.h>
#include<sys/utsname.h>
#include<string.h>
#include<sys/types.h>
#include<dirent.h>
#include<sys/stat.h>
#include<time.h>
#include<grp.h>

#define OPTION_A 0
#define OPTION_I 1
#define OPTION_L 2

int option = 0;//按位保存传入的选项
#define SETOPTION(option,val) (option)|=(1<<val)
#define ISSET(option,val) (option)&(1<<val)

//解析用户输入的选项信息
void Getoption(char* argv[],int argc)
{
	int i = 1;
	for(;i<argc;i++)
	{
		if(strncmp(argv[i],"-",1) != 0)
		{
			continue;//没有选项直接进入下一个选项
		}
		if(strstr(argv[i],"a") != NULL)
		{
			SETOPTION(option,OPTION_A);
		}
		else if(strstr(argv[i],"i") != NULL)
		{
			SETOPTION(option,OPTION_I);
		}
		else if(strstr(argv[i],"l") != NULL)
		{
			SETOPTION(option,OPTION_L);
		}
	}		
}
//输出文件详细信息之输出文件类型
void PrintType(struct stat st)
{
	if(S_ISDIR(st.st_mode))//管道文件
		printf("d");
	else
		printf("-");//普通文件
}

//输出文件详细信息之输出文件权限
void PrintMode(struct stat st)
{
	if(st.st_mode & S_IRUSR)
		printf("r");
	else
		printf("-");
	if(st.st_mode & S_IWUSR)
		printf("w");
	else
		printf("-");
	if(st.st_mode & S_IXUSR)
		printf("x");
	else
		printf("-");
	printf(" ");
}
void PrintFileInfo(char* path,char* file)
{
	char filename[128] = {0};
	strcpy(filename,path);
	strcat(filename,"/");
	strcat(filename,file);

	struct stat st;
	stat(filename,&st);

	PrintType(st);
	PrintMode(st);
	printf("%d ",st.st_nlink);//链接到该文件的硬链接数目
}

//根据类型指定文件名输出的颜色
void PrintFilename(char*path,char* file)
{
	char filename[128] = {0};
	strcpy(filename,path);
	strcat(filename,"/");
	strcat(filename,file);
	
	struct stat st;
	lstat(filename,&st);
	
	//普通文件
	if(S_ISREG(st.st_mode))
	{
		if(st.st_mode & S_IXUSR || st.st_mode & S_IXGRP
		   || st.st_mode & S_IXOTH)
		   {
			   printf("\033[1;32m%s\033[0m  ",file);//可执行为绿色
		   }
		else
		{
			printf("%s  ",file);//其余为黑色
		}
	}
	
	else if(S_ISDIR(st.st_mode))//目录文件为蓝色
	{
		printf("\033[1;34m%s\033[0m  ",file);
	}
	else if(S_ISFIFO(st.st_mode))//管道文件黑底黄字
	{
		printf("\033[40;33m%s\033[0m  ",file);
	}
	else//其余文件类型为黑色
	{
		printf("%s  ",file);
	}
}
//

//根据选项打印指定路径的内容
void PrintFile(char* path)
{
	int flag = 0;//kon制输出打印的换行符
	DIR* dir = opendir(path);//打开目录流
	assert(dir != NULL);

	struct dirent* dt = NULL;
	while(NULL != (dt = readdir(dir)))
	{
		//无a选项，但文件是隐藏文件，直接跳过
		if((!ISSET(option,OPTION_A)) && strncmp(dt->d_name,".",1) == 0)
		{
			continue;
		}
		if(ISSET(option,OPTION_I))
		{
			printf("%d ",dt->d_ino);
		}
		if(ISSET(option,OPTION_L))
		{
			PrintFileInfo(path,dt->d_name);
			flag = 1;//针对一个文件的所有详细信息输出过后再换行
		}

        //输出带颜色的文件
		PrintFilename(path,dt->d_name);
		if(flag)
		{
			printf("\n");
		}
	}
	if(!flag)
	{
		printf("\n");
	}
	closedir(dir);
}
	
int main(int argc,char* argv[])
{
	Getoption(argv,argc);

	int flag = 0;
	int i = 1;
	for(;i<argc;i++)
	{
		if(strncmp(argv[i],"-",1) == 0)
		{
			continue;
		}
		printf("%s :\n",argv[i]);
		PrintFile(argv[i]);//显示该路径下的所有文件
		flag = 1;
	}
	
	if(!flag)
	{
		char path[128] = {0};
		getcwd(path,127);
		PrintFile(path);
	}
	exit(0);
}
	
