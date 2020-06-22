#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>

#include<fcntl.h>
#include<sys/stat.h>

int main(int argc,char* argv[])
{
	if(argc < 3)
	{
		printf("cp errpr,paramter is not enough\n");
		exit(0);
	}
	
	int fr = open(argv[1],O_RDONLY);
	if(fr == -1)
	{
		perror("open error\n");
		exit(0);
	}
	
	struct stat st;
	int n = stat(argv[2],&st);
	char path[128] = {0};
	strcpy(path,argv[2]);
	
	if( n != -1 && S_ISDIR(st.st_mode))
	{
		char* p = argv[1]+strlen(argv[1]);
		while(p != argv[1] && *p != '/')
		{
			p--;
		}
		strcat(path,"/");
		strcat(path,p);
	}
	
	int fw = open(path,O_WRONLY | O_CREAT | O_TRUNC,0064);
	if(fw == -1)
	{
		perror("open2 error");
		exit(0);
	}
	
	while(1)
	{
		char buff[128] = {0};
		int num = read(fr,buff,127);
		if(num <= 0)
		{
			break;
		}
		
		int res = write(fw,buff,127);
		if(res <= 0)
		{
			break;
		}
	}
	close(fr);
	close(fw);
}
