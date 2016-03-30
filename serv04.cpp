#include <stdio.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

#define MAXN 16384  /*max #bytes client can request*/
int iChildren = 0;
int iListenFd;
typedef struct {
  pthread_t		thread_tid;		/* thread ID */
  long			thread_count;	/* # connections handled */
} Thread;
Thread	*tptr;		/* array of Thread structures; calloc'ed */


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

    for (int i = 0; i < iChildren; i++)
    {
    	printf("child %d, %ld connections\n", i, tptr[i].thread_count);
    }
    exit(0);
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

void* child_main(void* argc)
{	
	sockaddr_in oClientAddr;
	socklen_t iSockLen = sizeof(oClientAddr);
	int iConnFd;
	unsigned long index = (unsigned long)argc;
	
	for (; ; )
	{
		if ( (iConnFd = accept(iListenFd, (sockaddr*)&oClientAddr, &iSockLen)) <= 0)
		{
			printf("accept error! error=%s", strerror(errno));
			continue;
		}
		tptr[index].thread_count++;
		web_child(iConnFd);		
		close(iConnFd);
	}
	return NULL;
}

int thread_create(int i)
{
	pthread_create(&tptr[i].thread_tid, NULL, child_main, (void*)i);
	return 0;
}


int main(int argc, char* argv[])
{
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

	tptr = (Thread*)malloc(iChildren * sizeof(Thread));
	if (tptr == NULL)
	{
		printf("malloc error!\n");
		return -1;
	}
	memset(tptr, 0, sizeof(iChildren * sizeof(Thread)));

    printf("port=%d, children num=%d\n", oBindAddr.sin_port, iChildren);
    for (int i = 0; i < iChildren; i++)
    {
    	thread_create(i);
    }

    signal(SIGINT, sig_int);
    signal(SIGUSR1, sig_usr1);

    for(; ;)
    {
    	pause();
    }
    
}

