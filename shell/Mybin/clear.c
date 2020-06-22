#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>

int main(int argc,char* argv[],char* envp[])
{
	 printf("\033[2J\033[0;0H");
	return 0;
}
