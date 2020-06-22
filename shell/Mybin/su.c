#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include <shadow.h>
#include <pwd.h>
#include <termios.h>

int main(int argc, char *argv[])
{
	char *user = "root";
	if(argv[1] != NULL)
	{
		user = argv[1];
	}

	printf("Password: ");
	char password[128] = {0};

	//  先使得终端不回显
	struct termios oldterm, newterm;
	tcgetattr(0, &oldterm);  //  将当前终端的属性获取到oldterm
	newterm = oldterm;
	newterm.c_lflag &= ~ECHO;
	tcsetattr(0, TCSANOW, &newterm);  // 将终端的控制模式改为不回显以及非标准模式
	fgets(password,127,stdin);
	tcsetattr(0,TCSANOW,&oldterm);
	password[strlen(password)-1] = 0;


	struct spwd *sp = getspnam(user);//  /etc/shadow   普通用户没有权限调用此函数
	assert(sp != NULL);
	//printf("%s\n", sp->sp_pwdp);

	char salt[128] = {0}; //  $ID$密钥$
	int index = 0;
	int count = 0;
	char* p = sp->sp_pwdp;
	while(*p)
	{
		salt[index] = *p;
		if(salt[index] == '$')
		{
			count++;
			if(count == 3)
			{
				break;
			}
		}
		p++;
		index++;
	}

	char *mypasswd = (char*)crypt(password, salt);

	if(strcmp(mypasswd, sp->sp_pwdp) != 0)
	{
		printf("password is error,please again\n");
		exit(0);
	}

	pid_t pid = fork();
	assert(-1 != pid);
	if(pid == 0)
	{
		struct passwd *pw = getpwnam(user);  //  getpwuid(getuid())
		assert(pw != NULL);

		setenv("HOME",pw->pw_dir,1);

		//切换用户的操作
		setuid(pw->pw_uid);
		execl(pw->pw_shell, pw->pw_shell, (char*)0);
		perror("exec error: ");
		exit(0);
	}
	else
	{
		wait(NULL);
	}
	exit(0);
}
