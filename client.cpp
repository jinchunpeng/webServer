#include <stdio.h>
#include <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>   /* inet(3) functions */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define MAXN 16384		//bytes to request from server
#define MAXLINE 256


int main(int argc, char* argv[])
{
     int iChildren, iLoops, iBytes;
     int iFd, iIp, iPort;
     sockaddr_in oAddrSer;
   
     char szRequest[MAXLINE] = {'\0'};
     char szReply[MAXN];
     if (argc != 6)
     {
		printf("usage:client<IPaddr> <port> <children> <#loops/child> <#bytes/request>\n");
		return -1;
     }

     iChildren = atoi(argv[3]);
     iLoops    = atoi(argv[4]);
     iBytes    = atoi(argv[5]);
     iPort     = atoi(argv[2]);
     inet_pton(AF_INET, argv[1], &iIp);

     snprintf(szRequest, sizeof(szRequest), "%d\n", iBytes);
    
     //iFd = socket(AF_INET, SOCK_STREAM, 0);
     oAddrSer.sin_family = AF_INET;
     oAddrSer.sin_port	 = htons(iPort);
     oAddrSer.sin_addr.s_addr = iIp;       
     
     for (int i = 0; i < iChildren; i++)
     {
		if ( 0 == fork()) //child 
		{
		    for (int j = 0; j < iLoops; j++)
		    {
				iFd = socket(AF_INET, SOCK_STREAM, 0);
				if ( connect(iFd, (struct sockaddr*)&oAddrSer, sizeof(oAddrSer)) < 0 )
				{
				    printf("child %d loopth %d connect error!errno=%s\n", i, j, strerror(errno));
				}
				else
				{
				    int iRecvLen = 0;
				    send(iFd, szRequest, strlen(szRequest), 0);
				    if ( (iRecvLen = recv(iFd, szReply, sizeof(szReply), 0)) != iBytes)
				    {
						printf("child %d loopth %d recv=%d error = %s!\n", i, j, iRecvLen, strerror(errno));
				    }
				}
				close(iFd);
		    }
		    
		    printf("child %d done!\n", i);
		    
		    exit(0);
		    
		}	
     }
     while(wait(NULL) > 0)
		;
		
     if (errno != ECHILD) 	//errno==ECHILD已经没有子进程
     {
		printf("wait error!\n");
     }
     exit(0);
} 
