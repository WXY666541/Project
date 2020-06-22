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
 
#define CMDLEN 128
#define NUM 128
void PrintInfo()
{
	struct passwd* pw = getpwuid(getuid());
	assert(pw != NULL);
	
	struct utsname host;
	uname(&host);
	
	char path[CMDLEN] = {0};
	getcwd(path,CMDLEN-1);
	
	char* dirname = NULL;
	if(strcmp(path,pw->pw_dir) == 0)
	{
		dirname = "~";
	}
	else
	{
		dirname = path+strlen(path);
		while(*dirname != '/')
		{
			dirname--;
		}
		if(strlen(path) != 1)
		{
			dirname++;
		}
	}
	
	char flag = '$';
	if(getuid() == 0)
	{
		flag = '#';
	}
	printf("[%s@%s %s]%c",pw->pw_name,host.nodename,dirname,flag);
}
void CutCommand(char*cmd,char* cmdArr[])
{
	char* p = strtok(cmd," ");
	int index = 0;
	while(p != NULL && index < NUM)
	{
		cmdArr[index++] = p;
	    p = strtok(NULL," ");
	}
}
void MycdCommend(char* path)
{
	static char oldpwd[CMDLEN] = {0};

	if(path == NULL || strncmp(path,"~",1) == 0)
	{
		struct passwd* pw = getpwuid(getuid());
		assert(pw != NULL);
		path = pw->pw_dir;
	}
	else if(strncmp(path,"-",1) == 0)
	{
		if(strlen(oldpwd) == 0)
		{
			printf("cd error,OLDPWD NOT SET\n");
			return;
		}
		path = oldpwd;
	}
	char nowpwd[CMDLEN] = {0};
	getcwd(nowpwd,CMDLEN-1);
	
	if( chdir(path) == -1)
	{
		perror("cd ");
		return;
	}
	memset(oldpwd,0,CMDLEN);
	strcpy(oldpwd,nowpwd);
}	

void DealExec(char* cmdArr[])
{
	pid_t pid = fork();
	assert(pid != -1);
	if(pid == 0)
	{
		char file[CMDLEN] = {0};
		if(strstr(cmdArr[0],"/") != NULL)
		{
			strcpy(file,cmdArr[0]);
		}
		else
		{
			strcpy(file,"/home/user1/Desktop/shell/Mybin/");
			strcat(file,cmdArr[0]);
		}
		execv(file,cmdArr);
		perror("execv");
		exit(0);
	}
	else
	{
		wait(NULL);
	}
}
int main()
{
    while(1)
    {
		PrintInfo();
        char cmd[CMDLEN] = {0};    
        fgets(cmd,127,stdin);   
        cmd[strlen(cmd) - 1] = 0;    
 
		if(strlen(cmd) == 0)
		{
			continue;
		}
		
        char *cmdArr[NUM] = {0};
		CutCommand(cmd,cmdArr);
		
		if(strlen(cmdArr[0]) == 2 && strncmp(cmdArr[0],"cd",2)==0)
		{
			MycdCommend(cmdArr[1]);
		}
		else if(strlen(cmdArr[0]) == 4 && strncmp(cmdArr[0],"exit",4)==0)
		{
			exit(0);
		}
		else
		{
			DealExec(cmdArr);
		}
	}
}   
