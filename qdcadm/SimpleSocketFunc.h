#ifndef _SIMPLESOCKETFUNC_H_
#define _SIMPLESOCKETFUNC_H_


#ifdef _WIN32
    #include <winsock2.h>
    #define pthread_t HANDLE
    #define socklen_t int
#else
    #define SOCKET int
    #include <stdio.h>
    #include <string.h>
    #include <errno.h>
    #include <unistd.h>

    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/select.h>
#endif

void CloseSocket(int sockfd);


//成功返回一个listen的socketfd，返回<0表示失败
int Listen(int aPort,char * szErrInfo,const char * aIpAddr="");

//返回0表示超时返回，返回<0表示失败，>0，返回值是新连接的socketfd
int  Accept(int listensocket,struct sockaddr* pFrom ,socklen_t * pFromlen,int timeoutms,char * szErrInfo);

//读取指定长度的数据，如果没有读到，则一直等待
int SocketRead(SOCKET sockfd,char * p,int len,char * szErrInfo);

//写指定长度的数据，如果第一次没有写完，则继续写，直到写完
int SocketWrite(SOCKET sockfd,char * p,int len,char * szErrInfo);

//读取一个数据包，如果没有读到，则一直等待
int ReadPacket(SOCKET sockfd,char * p,char * szErrInfo);

//发送一个数据包，如果没有读到，则一直等待
int SendPacket(SOCKET sockfd,char * p,int len,char * szErrInfo);

//测试是否有socket数据到达
int TestCanRecv(SOCKET sockfd,int timeoutms,char * szErrInfo);

//
void GetSysErrorInfo(int err_no,char * szErrInfo);

#endif

