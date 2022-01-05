#ifndef _CADMCLIENT_H
#define _CADMCLIENT_H

#include "SimpleSocketFunc.h"
#include "CAdmMsg.h"

/*****
 * Admin Client session
 *****/
class CAdmClient
{
public:
    CAdmClient(std::string host, int port, std::string pwd);
    virtual ~CAdmClient()
    {
        if (m_sock >= 0) {
            CloseSocket(m_sock);
        }
    }

    const char * GetErrMsg() const
    {
        return m_err;
    }
    const SOCKET GetSock() const
    {
        return m_sock;
    }
    bool IsConnected() const
    {
        return m_sock >= 0;
    }
    bool SetTimeout(int timeoutS);

    int Login();
    int SendCommand(std::string cmd, const std::map<std::string, std::string>& params = {});

private:
    SOCKET m_sock;
    std::string m_pwd;
    char m_err[LEN_ERR_MSG];
};

#endif /* _CADMCLIENT_H */
