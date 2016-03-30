#include <stdio.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>

#define MAXN 16384  /*max #bytes client can request*/
int iChildren = 0;
static pid_t* tChildPid;
long* cptr;

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
	for (int i = 0; i < iChildren; i++)
	{
		kill(tChildPid[i], SIGTERM);
	}

	while(wait(NULL) > 0)
	{
		;
	}

	if (errno != ECHILD)
	{
		printf("wait error!\n");
	}
	
    pr_cpu_time();

    for (int i = 0; i < iChildren; i++)
    {
    	printf("child %d, %ld connections\n", i, cptr[i]);
    }
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

void child_main(int i, int iListenFd, socklen_t iSockLen)
{	
	sockaddr_in oClientAddr;
	int iConnFd;
	for (; ; )
	{
		if ( (iConnFd = accept(iListenFd, (sockaddr*)&oClientAddr, &iSockLen)) <= 0)
		{
			printf("accept error! error=%s", strerror(errno));
			return;
		}
		cptr[i]++;
		web_child(iConnFd);		
		close(iConnFd);
	}
}

int child_make(int i, int iListenFd, socklen_t iSockLen)
{
	pid_t pid;
	
	if ( (pid = fork()) > 0 )
	{
		return pid;
	}
	
	child_main(i, iListenFd, iSockLen);
	return 0;
}

long * meter(int nchildren)
{

	long	*ptr;

#ifdef	MAP_ANON
	ptr = (long*)mmap(0, nchildren*sizeof(long), PROT_READ | PROT_WRITE,
			   MAP_ANON | MAP_SHARED, -1, 0);
#else
	int		fd;
	fd = open("/dev/zero", O_RDWR, 0);

	ptr = (long*)mmap(0, nchildren*sizeof(long), PROT_READ | PROT_WRITE,
			   MAP_SHARED, fd, 0);
	close(fd);
#endif

	return (ptr);
}

int main(int argc, char* argv[])
{
    int iListenFd;
    sockaddr_in oBindAddr;
    sockaddr_in oCliAddr;
    socklen_t clilen;

    if (argc < 2)
    {
		printf("usage:Serv00 <port> <children>\n");
		return -1;
    }
    printf("argc =%d\n", argc);
    
    oBindAddr.sin_family = AF_INET;
    oBindAddr.sin_port	 = htons(atoi(argv[1]));
    oBindAddr.sin_addr.s_addr = 0;

    iListenFd = socket(AF_INET, SOCK_STREAM, 0);
    int iRet = bind(iListenFd,(struct sockaddr*) &oBindAddr, sizeof(oBindAddr));
    if (iRet != 0)
    {
		printf("bind error!\n");
		return -1;
    }
    listen(iListenFd, 1024);
   
    clilen = sizeof(oCliAddr);
    iChildren= atoi(argv[2]);
    tChildPid = (pid_t*)malloc( iChildren * sizeof(pid_t));
    cptr = meter(iChildren);

    printf("port=%d, children num=%d\n", oBindAddr.sin_port, iChildren);
    for (int i = 0; i < iChildren; i++)
    {
    	tChildPid[i] = child_make(i, iListenFd, clilen);
    }

    signal(SIGINT, sig_int);
    signal(SIGUSR1, sig_usr1);
    //signal(SIGCHLD, sig_chld);

    for(; ;)
    {
    	pause();
    }
    
}
