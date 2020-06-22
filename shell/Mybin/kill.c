#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<signal.h>

int main(int argc,char*argv[])
{
	if(argc < 2)
	{
		printf("please input processe pid again\n");
		exit(0);
	}

	int sigtype = 15;
	if(strncmp(argv[1],"-9",2) == 0)
	{
		sigtype = 9;
	}
	if(strncmp(argv[1],"-stop",5) == 0)
	{
		sigtype = 19;
	}

	int i = 1;
	for(;i<argc;i++)
	{
		if(i == 1 && strncmp(argv[i],"-",1)== 0)
		{
			continue;
		}
		int pid = 0;
		sscanf(argv[i],"%d",&pid);
		if(kill(pid,sigtype) == -1)
		{
			perror("kill over");
		}
	}
}



