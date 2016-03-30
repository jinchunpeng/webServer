#include <stdio.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>


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

void web_child(int iConnFd)
{
    char szLine[256], szResult[MAXN];
    int  iSend, iSendRet;
    for (; ; )
    {
		if (recv(iConnFd, szLine, sizeof(szLine), 0) == 0)
		{
		    return;
		}

		iSend = atoi(szLine);
		if ( (iSendRet = send(iConnFd, szResult, iSend, 0)) != iSend)
		{
			printf("iConnFd=%d send iSendRet=%d errnoStr=%s\n", iConnFd, iSendRet, strerror(errno));
		}
    }
}

void* child_thread(void* argv)
{
	pthread_detach(pthread_self());
	printf("connectfd=%ld\n", (unsigned long int)argv);
	web_child((unsigned long int)argv);
	printf("connect end=%ld!\n", (unsigned long int)argv);
	close((unsigned long int)argv);
	return (NULL);
}

int main(int argc, char* argv[])
{
    int iListenFd;
    sockaddr_in oBindAddr;
    sockaddr_in oCliAddr;
    socklen_t clilen;
    pthread_t iThread;

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
    
    clilen = sizeof(oCliAddr);
    for (; ; )
    {
		int iConnFd = accept(iListenFd, (struct sockaddr*)&oCliAddr, &clilen);
	    if (iConnFd > 0)
		{
		  //不能传局部变量的地址进去
		  //if ( pthread_create(&iThread, NULL, child_thread, (void *)&iConnFd) != 0)
		  if ( pthread_create(&iThread, NULL, child_thread, (int *)iConnFd) != 0)
		  {
		  	   printf("pthread_create error!\n");
		  }
		}	
		else
		{
		    if (iConnFd == EINTR)
		    {
				continue;
		    }   
		    else
		    {
				printf("accept error!\n");
		    }
		}   
    }
    
}

