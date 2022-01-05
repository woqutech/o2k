#define SYS_ERRMSG_MAX_SIZE 300
#include "SimpleSocketFunc.h"

int Listen(int aPort,char * szErrInfo,const char * aIpAddr)
{
	struct sockaddr_in my_addr;
	struct linger li;
	char errtext[512];
	int option=1;
    int listensockfd=0;

	li.l_onoff = 1;
	li.l_linger = 0;


	//strcpy(m_ipaddr,aIpAddr);
    //m_port=aPort;

	//0.init
	memset ((char *)&my_addr, 0, sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(aPort);
	if (aIpAddr == NULL || aIpAddr[0]==0)
	{
		my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}else
	{
		my_addr.sin_addr.s_addr = inet_addr(aIpAddr);
	}

	//1.create
	listensockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listensockfd==-1)
	{
		GetSysErrorInfo(errno,szErrInfo);
        return -1;
	}

	setsockopt(listensockfd,SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option));
	setsockopt(listensockfd,SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(li));

	//2.bind
	int ret = bind(listensockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
	if (ret==-1)
	{
        char szSysErrorInfo[SYS_ERRMSG_MAX_SIZE];
        GetSysErrorInfo(errno,szSysErrorInfo);
		if (aIpAddr[0]==0)
	    {

            sprintf(szErrInfo,"bind error in all local ip, port %d: %s",aPort,szSysErrorInfo);
		}
		else
		{
		    sprintf(szErrInfo,"bind error in ip %s, port %d: %s",aIpAddr,aPort,szSysErrorInfo);
		}
		return -1;
	}
	//3.listen
	ret = listen(listensockfd, 10);
	if (ret==-1)
	{
		char szSysErrorInfo[SYS_ERRMSG_MAX_SIZE];
        GetSysErrorInfo(errno,szSysErrorInfo);
		if (aIpAddr[0]==0)
	    {
		     sprintf(szErrInfo,"listen error in all local ip, port %d: %s",aPort,szSysErrorInfo);
		}
		else
		{
		     sprintf(szErrInfo,"listen error in ip %s, port %d: %s",aIpAddr,aPort,szSysErrorInfo);
		}
		return -1;
    }
	return listensockfd;
}

/* 当timeoutms为-1时，为永不超时*/
int  Accept(int listensocket,struct sockaddr* pFrom ,socklen_t * pFromlen,int timeoutms,char * szSysErrorInfo)
{
	int nfds;

	nfds=listensocket+1;

	fd_set tfds,acceptready;
	FD_ZERO(&acceptready);
	FD_SET(listensocket, &acceptready);

	fd_set	aerrfds,errfds;
	FD_ZERO(&aerrfds);
	FD_SET( listensocket ,&aerrfds);

	int ret=0;
	if(timeoutms == -1) //为不超时，一直等下去
	{
		while(true)
		{
			ret = select(nfds,&acceptready,(fd_set *)0,(fd_set *)0,(struct timeval *)0);
			if (ret>=0)
			{
			    break;
			}
#ifndef WIN32
			if (ret<0&&errno==EINTR)
			{
			    continue;
			}
#endif
			if(ret<0)
            {
		        GetSysErrorInfo(errno,szSysErrorInfo);
                return -1;
			}
		}
	}
	else
	{
		struct timeval		timeout;
		timeout.tv_sec = timeoutms / 1000;
		timeout.tv_usec = (timeoutms % 1000) * 1000;

		while(true)
		{
			memcpy(&tfds,&acceptready,sizeof(acceptready));
			memcpy(&errfds,&aerrfds,sizeof(errfds));

			ret = select(nfds,&tfds,(fd_set *)0,&errfds,&timeout);
			if(errno!=0)
            {
		        GetSysErrorInfo(errno,szSysErrorInfo);
			}
#ifndef WIN32
			if (ret<0&&errno==EINTR)
			{
			    continue;
			}
            else if(ret<0 && errno != EINTR)
            {
                return ret;
            }
#endif

			if(FD_ISSET(listensocket ,&errfds))
			{
		        //PrtLog(ERRLOG,"Socket select error: %s",syserr.c_str());
				GetSysErrorInfo(errno,szSysErrorInfo);
                return -1;
			}

			if (ret>=0)
			{
			    break;
			}
		}
	}

	if (ret==0)
	{
	    return 0;
    }

	if(FD_ISSET(listensocket, &acceptready))
	{
		int acceptfd = accept(listensocket,(struct sockaddr*)pFrom,pFromlen);
		if(errno!=0)
		{
		    GetSysErrorInfo(errno,szSysErrorInfo);
		}

		if (acceptfd==-1)
		{
#ifndef WIN32
			if (errno==EINTR)
			{
			    return 0;
			}
#endif
			//TchPrtLog(ERRLOG,"Socket accept error: %s",errtext);
		    return -1;
		}
		return acceptfd;
	}

	return 0;
}


void CloseSocket(int sockfd)
{
	if (sockfd!=-1)
	{
#ifdef WIN32
		shutdown(sockfd,SD_BOTH);
		closesocket(sockfd);
#else
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
#endif
	}
	sockfd=-1;
}


int SocketRead(SOCKET sockfd,char * p,int len,char * szErrInfo)
{
    int n=0;
    int readnums;
    char errtext[512];

    while(n<len)
    {
        readnums = recv(sockfd,&p[n],len -n,0);
        if(readnums < 0)
        {
#ifndef WIN32
            if(errno==EINTR)
            {
                continue;
            }
#endif
            GetSysErrorInfo(errno,szErrInfo);
            //GetLastErrText(errtext,512);
            //TchPrtLog(ERRLOG,"Socket recv error: %s",errtext);
            return -1;
        }

        if(readnums==0)
        {
            return 0;
        }

        n+=readnums;
    }
    return n;
}

int SocketWrite(SOCKET sockfd,char * p,int len,char * szErrInfo)
{
    int n=0;
    int sendnums;
    char errtext[512];

    while(n<len)
    {
        sendnums = send(sockfd,&p[n],len -n,0);
        if(sendnums <= 0)
        {
#ifndef WIN32
            if(errno==EINTR)
            {
                continue;
            }
#endif
            GetSysErrorInfo(errno,szErrInfo);
            return -1;
        }
        n+=sendnums;
    }
    return n;
}



int ReadPacket(SOCKET sockfd,char * p,char * szErrInfo)
{
    int header;
    int len;
    int iRet;

    iRet=SocketRead(sockfd,(char *)&header,4,szErrInfo);
    if(iRet<0)
    {
        GetSysErrorInfo(errno,szErrInfo);
        return iRet;
    }
    len=ntohl(header);
    if(len<0)
    {
         sprintf(szErrInfo,"Invalid packet data format!");
         return -1;
    }
    iRet=SocketRead(sockfd,p,len,szErrInfo);
    return iRet;
}

int SendPacket(SOCKET sockfd,char * p,int len,char * szErrInfo)
{
    int header;
    int iRet;
    header=htonl(len);
    iRet=SocketWrite(sockfd,(char *)&header,4,szErrInfo);
    if(iRet<0)
    {
        return iRet;
    }
    iRet=SocketWrite(sockfd,(char *)p,len,szErrInfo);
    return iRet;
}


/*测试socket是否有数据到达 */
int TestCanRecv(SOCKET sockfd,int timeoutms,char * szErrInfo)
{
    if (sockfd<0)
    {
        return -1;
    }

    int nfds;

    nfds = sockfd+1;

    fd_set  arfds,rfds;
    FD_ZERO(&arfds);
    FD_SET( sockfd ,&arfds);

    fd_set  aerrfds,errfds;
    FD_ZERO(&aerrfds);
    FD_SET( sockfd ,&aerrfds);

    struct timeval  timeout;
    if (timeoutms!=-1)
    {
        timeout.tv_sec = timeoutms / 1000;
        timeout.tv_usec = (timeoutms % 1000) * 1000;
    }

    int ccode=0;
    while(true)
    {
        memcpy(&rfds,&arfds,sizeof(rfds));
        memcpy(&errfds,&aerrfds,sizeof(errfds));

        if(timeoutms == -1)
        {
            ccode = select(nfds,&rfds,(fd_set *)0,&errfds,(struct timeval *)0);
        }
        else
        {
            ccode = select(nfds,&rfds,(fd_set *)0,&errfds,&timeout);
        }

        //超时
        if(ccode == 0)
        {
            return 0;
        }

        if(ccode < 0)
        {
#ifndef WIN32
            if(errno==EINTR)
            {
                continue;
            }
#endif
            GetSysErrorInfo(errno,szErrInfo);
            return -1;
        }

        //读失败
        if(FD_ISSET(sockfd ,&errfds))
        {
            GetSysErrorInfo(errno,szErrInfo);
            return -1;
        }

        if(FD_ISSET(sockfd ,&rfds))
        {
            return 1;
        }
    }

    return 0;
}


void GetSysErrorInfo(int err_no,char * szErrInfo)
{
    char * errmsg;
    int headlen;

    //memset(szErrMsg,0,SYS_ERRMSG_MAX_SIZE);
    headlen=sprintf(szErrInfo,"errno=%d, ",err_no);

#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
    strerror_r(err_no,&szErrInfo[headlen],SYS_ERRMSG_MAX_SIZE);
#else
	errmsg=strerror_r(err_no,szErrInfo,SYS_ERRMSG_MAX_SIZE);
    strncpy(&szErrInfo[headlen],errmsg,SYS_ERRMSG_MAX_SIZE-1);
#endif
}



