#include <stdio.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/*
In UNIX System terminology, a process that has terminated,but whose parent has not yet waited for it, is called a zombie. 
在UNIX 系统中，一个进程结束了，但是他的父进程没有等待(调用wait / waitpid)他， 
那么他将变成一个僵尸进程。 但是如果该进程的父进程已经先结束了，
那么该进程就不会变成僵尸进程， 因为每个进程结束的时候，系统都会扫
描当前系统中所运行的所有进程， 看有没有哪个进程是刚刚结束的这个进
程的子进程，如果是的话，就由Init 来接管他，成为他的父进程……
*/

#define MAXN 16384  /*max #bytes client can request*/
void pr_cpu_time(void)
{
    double	    user, sys, sum;
    struct rusage   myusage, childusage;

    if (getrusage(RUSAGE_SELF, &myusage) < 0)
	printf("getrusage error\n");
    if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
	printf("getrusage error\n");

    user = (double) myusage.ru_utime.tv_sec +
		    myusage.ru_utime.tv_usec/1000000.0;
    user += (double) childusage.ru_utime.tv_sec +
		     childusage.ru_utime.tv_usec/1000000.0;
    sys = (double) myusage.ru_stime.tv_sec +
		   myusage.ru_stime.tv_usec/1000000.0;
    sys += (double) childusage.ru_stime.tv_sec +
		    childusage.ru_stime.tv_usec/1000000.0;

	sum = user + sys;
    printf("\nuser time = %g, sys time = %g, sum time=%g\n", user, sys, sum);

}

void sig_usr1(int iSigno)
{
    pr_cpu_time();
}

void sig_int(int iSigno)
{   
    pr_cpu_time();
    exit(0);
}

void sig_chld(int iSigno)
{
    pid_t   pid;
    int	    stat;

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) 
    {
		printf("child %d terminated\n", pid);
    }
    return;
}

void web_child(int iConnFd)
{
    char szLine[256], szResult[MAXN];
    int  iSend;
    for (; ; )
    {
		if (recv(iConnFd, szLine, sizeof(szLine), 0) == 0)
		{
		    return;
		}

		iSend = atoi(szLine);
		send(iConnFd, szResult, iSend, 0);
    }
}

int main(int argc, char* argv[])
{
    int iListenFd;
    pid_t tChildPid;
    sockaddr_in oBindAddr;
    sockaddr_in oCliAddr;
    socklen_t clilen;

    if (argc < 2)
    {
		printf("usage:Serv00 <port>");
		return -1;
    }
    
    oBindAddr.sin_family = AF_INET;
    oBindAddr.sin_port	 = htons(atoi(argv[1]));
    oBindAddr.sin_addr.s_addr = 0;

    iListenFd = socket(AF_INET, SOCK_STREAM, 0);
    int iRet = bind(iListenFd,(struct sockaddr*) &oBindAddr, sizeof(oBindAddr));
    if (iRet != 0)
    {
		printf("bind error!");
		return -1;
    }

    listen(iListenFd, 1024);
    signal(SIGINT, sig_int);
    signal(SIGUSR1, sig_usr1);
    signal(SIGCHLD, sig_chld);
    
    clilen = sizeof(oCliAddr);
    for (; ; )
    {
		int iConnFd = accept(iListenFd, (struct sockaddr*)&oCliAddr, &clilen);
	    if (iConnFd > 0)
		{
		    if ( ( tChildPid = fork()) == 0 )
		    {
				close(iListenFd);
				web_child(iConnFd);
				exit(0);
		    }
		    close(iConnFd);
		}	
		else
		{
		    if (iConnFd == EINTR)
		    {
				continue;
		    }   
		    else
		    {
				printf("accept error!");
		    }
		}   
    }
    
}
