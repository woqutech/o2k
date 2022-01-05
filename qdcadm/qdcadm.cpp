#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include <errno.h>
#include <strings.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "CAdmClient.h"

#define _GITVERSION "3.0"

static void usage(const char *name)
{
    FILE *of = stdout;
    fprintf(of, "%s %s build %s %s\n", name, _GITVERSION, __DATE__, __TIME__);
    fprintf(of, "Usange: %s <assembler_server_ip:port> <password> <command> [args]\n", name);
    fprintf(of, "\tcommand:\n");
    fprintf(of, "\t\tlist : list all commands which assembler server supported.\n");
    fprintf(of, "\tfor Example:\n");
    fprintf(of, "\t\t %s 127.0.0.1:9876 qdecoder list\n", name);
    fprintf(of, "\t\t %s 127.0.0.1:9876 qdecoder get_stat\n", name);
    fprintf(of, "\t\t %s 127.0.0.1:9876 qdecoder set_conf print_dict 1\n", name);
    exit(0);
}

static std::string strLower(std::string str)
{
    std::string tmp;
    tmp.reserve(str.size());
    for (auto c : str) {
        tmp += std::tolower(c);
    }
    return tmp;
}

static struct tm nowTm()
{
    time_t now;
    struct tm tmNow;

    now = time(NULL);
    localtime_r(&now, &tmNow);

    return tmNow;
}

std::string nowStr()
{
    char buf[64];
    auto tmNow = nowTm();
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
        tmNow.tm_year+1900, tmNow.tm_mon+1, tmNow.tm_mday, tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);
    return std::string(buf);
}

static int ExecuteCommand(const char *szIpAddr, int iPort, const char *pwd, int timeoutS, const std::string& cmd, const std::map<std::string, std::string>& params)
{
    int    rc = 0;

    // connect to server
    CAdmClient ses{szIpAddr, iPort, pwd};
    if (ses.GetSock() < 0) {
        printf("Connect to server fail: %s\n", ses.GetErrMsg());
        return 1;
    }

    if (timeoutS > 0) {
        ses.SetTimeout(timeoutS);
    }

    // send command
    rc = ses.SendCommand(cmd, params);
    if (rc != 0) {
        printf("send command fail: %s\n", ses.GetErrMsg());
        return 1;
    }

    // read packet
    CMsgRW msgrw{ses.GetSock()};
    rc = msgrw.ReadPacket();
    if (rc <= 0) {
        printf("read result fail: %s\n", msgrw.GetErrMsg());
        return 1;
    }

    // parse msg header
    admin::MsgHeaderWrap hdr;
    rc = msgrw.ParseMsgHeader(hdr);
    if (rc != 0) {
        printf("parse msg header fail: %s\n", msgrw.GetErrMsg());
        return 1;
    }

    // check MsgResult
    admin::MsgResult msgResult;
    if (hdr.msg_header().msg_type() == admin::MsgType::Result) {
        if (msgrw.ParseResult(msgResult)) {
            printf("parse MsgResult fail: %s\n", msgrw.GetErrMsg());
            return 1;
        }
        if (!StrCaseEq(msgResult.rc().c_str(), RC_OK)) {
            printf("%s\n", msgResult.msg().c_str());
            return 1;
        } else {
            printf("[OK] %s: %s\n", nowStr().c_str(), msgResult.msg().c_str());

            // show rt_pairs:
            for (auto& pair : msgResult.rt_pairs()) {
                printf("%20s : %s\n", pair.key().c_str(), pair.val().c_str());
            }
        }
    }

    // read more info for different command

    if (StrCaseEq(cmd.c_str(), CMD_GET_METRICS)) {
        admin::MsgMetrics msgMetrics;
        const char *msg = NULL;
        int len = 0;
        if (!msgrw.GetPacket(&msg, &len)) {
            printf("no message body\n");
            return -1;
        }
        if (!msgMetrics.ParseFromArray(msg, len) &&
                !msgMetrics.ParseFromString(msg)) {
            printf("can not parse to MsgMetrics\n");
            return -1;
        }

        //std::string str = msgMetrics.SerializeAsString();
        printf("receive metrics:\n  total_commit: %.0f\n", msgMetrics.trans_commit_total());
        for (auto proc : msgMetrics.trans_begin_proc()) {
            printf("  proc %ld commit: %.f\n", proc.key(), proc.val());
        }
    }

    return 0;
}

int main(int argc, char * argv[])
{
    char * p;
    char * szIpAddr;
    char * szPort;
    char * pwd;
    int iPort;

    if (argc < 4) {
        usage(argv[0]);
    }

    // parse ip:port
    szIpAddr = argv[1];
    p = strstr(argv[1], ":");
    if (p == NULL) {
        printf("Bad format ip address and port:%s\n", szIpAddr);
        return 1;
    }
    p[0] = 0;
    szPort = &p[1];
    iPort = atoi(szPort);
    pwd = argv[2];

    // parse command:
    std::string cmd = argv[3];

    // parse parameters for command
    std::map<std::string, std::string> params;
    char pname[128];
    for (int i = 4; i < argc; i++) {
        sprintf(pname, "%d", i-3);
        params[pname] = argv[i];
    }

    int nLoop = 1;
    int timeoutS = 5;

    // preprocess for some specific commands
    if (strLower(cmd) == CMD_GET_PERF) {
        if (params.size() < 1) {
            printf("missing argument: interval [loop times]\n");
            return 1;
        }
        auto itv = atoi(params["1"].c_str());
        if (itv < 1 || itv > 300) {
            printf("invalid interval, must be in the range 1~300\n");
            return 1;
        }
        if (params.size() > 1) {
            auto loop = atoi(params["2"].c_str());
            if (loop > 0) {
                nLoop = loop;
            }
        }

        timeoutS = 300;

        printf(CMD_GET_PERF " will run %d times, each time will wait %d seconds ...\n", nLoop, itv);
    }

    for (int i=0; i<nLoop; i++) {
        ExecuteCommand(szIpAddr, iPort, pwd, timeoutS, cmd, params);
    }

    return 0;
}
