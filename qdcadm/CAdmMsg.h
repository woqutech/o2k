
#ifndef _CADMIN_MSG_H
#define _CADMIN_MSG_H

#include "SimpleSocketFunc.h"
#include "AdminMsg.pb.h"

#define LEN_ERR_MSG  (512)
#define LEN_ADM_PKG  (1024)
#define MAX_ADM_PKG  (1<<20) // 1M bytes

// MsgResult.rc:
#define RC_OK       "OK"
#define RC_ERR      "error"

// MsgResult.msg:
#define RMSG_PONG               "pong"
#define RMSG_WELCOME            "welcome"
#define RMSG_INV_PKG            "invalid package"
#define RMSG_INV_MSG_HDR        "invalid message header"
#define RMSG_INV_MSG_BDY        "invalid message body"
#define RMSG_UNKNOW_MSG         "unknow message type"
#define RMSG_NOT_CMD            "not a command message"
#define RMSG_NOT_RESULT         "not a result message"
#define RMSG_NOT_LOGIN          "not login command"
#define RMSG_NOT_SUPPORT_CMD    "not supported command"
#define RMSG_MIS_PWD            "missing password"
#define RMSG_INV_PWD            "given a invalid password"
#define RMSG_PACK_FAIL          "pack message fail"
#define RMSG_SEND_FAIL          "send message fail"

// MsgCommand.cmd:
#define CMD_LOGIN               "login"
#define CMD_PING                "ping"
#define CMD_LIST                "list"
#define CMD_GET_START_SCN       "get_start_scn"
#define CMD_GET_CURR_SCN        "get_current_scn"
#define CMD_GET_BINLOG_SCN      "get_binlog_scn"
#define CMD_GET_BINLOG_LISTEN   "get_binlog_listen"
#define CMD_GET_PARSER_LISTEN   "get_parser_listen"
#define CMD_GET_CONF_NODE_NAME  "get_conf_node_name"
#define CMD_GET_DB_INFO         "get_db_info"
#define CMD_GET_SYNC_TAB_NUM    "get_sync_table_num"
#define CMD_GET_SYNC_PART_NUM   "get_sync_partition_num"
#define CMD_GET_SYNC_TAB_INFO   "get_sync_table_info" // table name ...
#define CMD_GET_DICT            "get_dict"
#define CMD_GET_LOG_LEVEL       "get_log_level"
#define CMD_GET_METRICS         "get_metrics"
#define CMD_RESET_STAT          "reset_stat"
#define CMD_RESET_STATISTICS    "reset_statistics"
#define CMD_GET_STAT            "get_stat"
#define CMD_GET_STATISTICS      "get_statistics"
#define CMD_GET_PERF            "get_perf"
#define CMD_GET_SYNC_DELAY      "get_sync_delay"
#define CMD_GET_CONF            "get_conf"
#define CMD_SET_CONF            "set_conf"
#define CMD_WRITE_CONF          "write_conf" // write conf info file
#define CMD_WRITE_DICT          "write_dict" // write dict into file
#define CMD_WRITE_CONF_DICT     "write_conf_dict" // write both conf and dict into file
#define CMD_WRITE_DICT_CONF     "write_dict_conf"

#define StrCaseEq(str1, str2) (strcasecmp(str1,str2)==0)


namespace admin = com::woqutech::admin;

class CMsgRW
{
public:
    CMsgRW(SOCKET sock) : m_sock(sock)
    {
        m_buf[0] = '\0';
        m_errbuf[0] = '\0';
        m_msgbuf = m_buf;
        m_lenbuf = 0;
        m_lenmsg = 0;
    }
    virtual ~CMsgRW()
    {
        if (m_lenbuf > 0 && m_msgbuf != NULL && m_msgbuf != m_buf) {
            delete []m_msgbuf;
            m_msgbuf = NULL;
            m_lenbuf = 0;
        }
    }

    const char * GetErrMsg() const
    {
        return m_errbuf;
    }
    void Clear(bool bFreeBuf = false);

    admin::MsgHeader *MakeMsgHdr(admin::MsgType msgType) const;

    int ReadPacket();
    int SendPacket(const char* pkg, int len);
    bool GetPacket(const char **buf, int *len) const;

    int ParseMsgHeader(admin::MsgHeaderWrap& msgHdr);
    int ParseCommand(admin::MsgCmd& msgCmd);
    int ParseResult(admin::MsgResult& msgResult);

    int ReadCommand(admin::MsgCmd& msgCmd);
    int ReadResult(admin::MsgResult& msgResult);

    int ReplyResult(std::string rc, std::string msg);
    int ReplyResult(std::string rc, std::string msg, const std::map<std::string, std::string>& apdmsg);
    int ReplyErr(std::string msg)
    {
        return ReplyResult(RC_ERR, msg);
    }
    int ReplyOK(std::string msg)
    {
        return ReplyResult(RC_OK, msg);
    }

    int ExecCmdResult(const admin::MsgCmd& cmd, admin::MsgResult& result);

private:
    SOCKET m_sock;
    char m_errbuf[LEN_ERR_MSG + 2];
    char m_buf[LEN_ADM_PKG + 2];
    char *m_msgbuf;
    size_t m_lenbuf;
    size_t m_lenmsg;
};

#endif /* _CADMIN_MSG_H */
