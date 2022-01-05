#include "CAdmClient.h"


CAdmClient::CAdmClient(std::string host, int port, std::string pwd) : m_pwd(pwd)
{
    // init fields

    m_err[0] = '\0';
    m_sock = -1;

    // connect to server

    struct timeval timeout = {5,0};
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if ((sin.sin_addr.s_addr = inet_addr(host.c_str())) == INADDR_NONE) {
        snprintf(m_err, LEN_ERR_MSG, "Bad format ip address:%s\n", host.c_str());
        return;
    }

    m_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (m_sock < 0) {
        snprintf(m_err, LEN_ERR_MSG, "can't create socket:%s\n", strerror(errno));
        return;
    }

    if (connect(m_sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
        snprintf(m_err, LEN_ERR_MSG, "Can not connect to %s:%d,error:%s\n",
                 host.c_str(), port, strerror(errno));
        CloseSocket(m_sock);
        m_sock = -1;
    } else {
        setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
        setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    }
}

bool CAdmClient::SetTimeout(int timeoutS)
{
    int rc = 0;
    struct timeval timeout = {timeoutS, 0};

    rc += setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
    rc += setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    return rc == 0;
}


/*****
 * login for check password
 *****/
int CAdmClient::Login()
{
    int rc = 0;
    CMsgRW msgrw{m_sock};
    admin::MsgCmd msgCmd;
    admin::MsgResult msgResult;

    if (!IsConnected()) {
        snprintf(m_err, LEN_ERR_MSG, "not connected to Admin Server");
        return -1;
    }

    msgCmd.set_allocated_msg_header(msgrw.MakeMsgHdr(admin::MsgType::Command));
    msgCmd.set_cmd(CMD_LOGIN);
    auto pair = msgCmd.add_params();
    pair->set_key("password");
    pair->set_val(m_pwd);

    rc = msgrw.ExecCmdResult(msgCmd, msgResult);
    if (rc) {
        snprintf(m_err, LEN_ERR_MSG, "%s", msgrw.GetErrMsg());
        return rc;
    }

    if (!StrCaseEq(msgResult.rc().c_str(), RC_OK)) {
        snprintf(m_err, LEN_ERR_MSG, "%s", msgResult.msg().c_str());
        return -1;
    }

    return 0;
}

/*****
 * execute a user command
 *****/
int CAdmClient::SendCommand(std::string cmd, const std::map<std::string, std::string>& params)
{
    int rc = 0;

    // login first
    rc = Login();
    if (rc) {
        return rc;
    }

    CMsgRW msgrw{m_sock};
    admin::MsgCmd msgCmd;
    msgCmd.set_allocated_msg_header(msgrw.MakeMsgHdr(admin::MsgType::Command));
    msgCmd.set_cmd(cmd);

    for (auto kv : params) {
        auto pair = msgCmd.add_params();
        pair->set_key(kv.first);
        pair->set_val(kv.second);
    }

    std::string pkg;
    if (!msgCmd.SerializeToString(&pkg)) {
        snprintf(m_err, LEN_ERR_MSG, "serialize protobuf fail");
        return -1;
    }

    rc = msgrw.SendPacket(pkg.c_str(), pkg.length());
    if (rc <= 0) {
        return -1;
    }

    return 0;
}
