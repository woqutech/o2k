#include "CAdmMsg.h"

void CMsgRW::Clear(bool bFreeBuf)
{
    m_lenmsg = 0;
    m_errbuf[0] = '\0';
    if (bFreeBuf) {
        if (m_lenbuf > 0 && m_msgbuf != NULL && m_msgbuf != m_buf) {
            delete []m_msgbuf;
            m_msgbuf = m_buf;
            m_lenbuf = 0;
        }
    }
}


//#define SHANG_DEBUG (1)

#ifdef SHANG_DEBUG
static void dumpbin(char *data, int len, char *label)
{
    printf("===== %s: dump %d =====\n", label, len);
    for (int i = 0; i < len; i++) {
        printf("%02x ", (unsigned char)data[i]);
        if ((i + 1) % 8 == 0) {
            printf("  ");
        }
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
    fflush(stdout);
}
#  define DUMPBIN(data, len, label) dumpbin(data, len, label)
#else
#  define DUMPBIN(data, len, label)
#endif

/*****
 * read packet from net and save it into m_msgbuf.
 *****/
int CMsgRW::ReadPacket()
{
    uint32_t header;
    int len;
    int iRet;

    // clear old data if exists
    Clear();

    // read package header
    iRet = SocketRead(m_sock, (char *)&header, 4, m_errbuf);
    if (iRet < 0) {
        return iRet;
    }
    len = ntohl(header);
    if (len < 0) {
        snprintf(m_errbuf, LEN_ERR_MSG, "Invalid packet data format!");
        return -1;
    } else if (len > MAX_ADM_PKG) {
        snprintf(m_errbuf, LEN_ERR_MSG, "message size is too long! %d", len);
        return -1;
    } else if (len > LEN_ADM_PKG) {
        if (len > m_lenbuf) { // need allocate large buffer
            if (m_lenbuf > 0 && m_msgbuf != NULL && m_msgbuf != m_buf) {
                delete []m_msgbuf;
            }
            m_lenbuf = len + 2;
            m_msgbuf = new char[m_lenbuf];
        }
    }

    // read package payload
    iRet = SocketRead(m_sock, m_msgbuf, len, m_errbuf);
    m_msgbuf[len] = '\0';
    if (iRet != len) {
        snprintf(m_errbuf, LEN_ERR_MSG, "can not read enough data");
        return -1;
    }
    m_lenmsg = iRet;

    DUMPBIN(m_msgbuf, m_lenmsg, "recv pkg");

    return iRet;
}

/*****
 * send packet to net, return wrote size.
 *****/
int CMsgRW::SendPacket(const char* pkg, int len)
{
    DUMPBIN((char *)pkg, len, "send pkg");
    int wLen = ::SendPacket(m_sock, (char *)pkg, len, m_errbuf);
    return wLen;
}

/*****
 * return packet buffer
 *****/
bool CMsgRW::GetPacket(const char ** buf, int* len) const
{
    if (m_lenmsg <= 0) {
        return false;
    }
    *buf = m_msgbuf;
    *len = m_lenmsg;
    return true;
}

/*****
 * new a msg header. if it is not added into MsgXXX, caller must delete it.
 *****/
admin::MsgHeader *
CMsgRW::MakeMsgHdr(admin::MsgType msgType) const
{
    auto msgHdr = new admin::MsgHeader{};
    msgHdr->set_msg_type(msgType);
    return msgHdr;
}

/*****
 * parse MsgHeaderWrap from m_msgbuf.
 *****/
int CMsgRW::ParseMsgHeader(admin::MsgHeaderWrap& msgHdr)
{
    if (m_lenmsg <= 0) {
        return -1;
    }
    if (!msgHdr.ParseFromString(m_msgbuf) &&
            !msgHdr.ParseFromArray(m_msgbuf, m_lenmsg)) {
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_INV_MSG_BDY);
        return -1;
    }
    return 0;
}

/*****
 * parse MsgCmd from m_msgbuf.
 *****/
int CMsgRW::ParseCommand(admin::MsgCmd& msgCmd)
{
    if (m_lenmsg <= 0) {
        return -1;
    }
    if (!msgCmd.ParseFromString(m_msgbuf) &&
            !msgCmd.ParseFromArray(m_msgbuf, m_lenmsg)) {
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_INV_MSG_BDY);
        return -1;
    }
    return 0;
}

/*****
 * parse MsgResult from m_msgbuf.
 *****/
int CMsgRW::ParseResult(admin::MsgResult& msgResult)
{
    if (m_lenmsg <= 0) {
        return -1;
    }
    if (!msgResult.ParseFromString(m_msgbuf) &&
            !msgResult.ParseFromArray(m_msgbuf, m_lenmsg)) {
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_INV_MSG_BDY);
        return -1;
    }
    return 0;
}

/*****
 * read packet from net, and parse MsgCmd.
 *****/
int CMsgRW::ReadCommand(admin::MsgCmd& msgCmd)
{
    if (ReadPacket() <= 0) {
        return -1;
    }

    admin::MsgHeaderWrap hdr;
    if (!hdr.ParseFromString(m_msgbuf) &&
            !hdr.ParseFromArray(m_msgbuf, m_lenmsg)) {
        // not a valid msg header
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_INV_MSG_HDR);
        return -1;
    }

    if (hdr.msg_header().msg_type() != admin::MsgType::Command) {
        // not a command message
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_NOT_CMD);
        return -1;
    }

    msgCmd.Clear();
    if (!msgCmd.ParseFromString(m_msgbuf) &&
            !msgCmd.ParseFromArray(m_msgbuf, m_lenmsg)) {
        // parser command message error
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_INV_MSG_BDY);
        return -1;
    }

    return 0;
}

/*****
 * read packet from net, and parse MsgResult.
 *****/
int CMsgRW::ReadResult(admin::MsgResult& msgResult)
{
    if (ReadPacket() <= 0) {
        return -1;
    }

    admin::MsgHeaderWrap hdr;
    if (!hdr.ParseFromString(m_msgbuf) &&
            !hdr.ParseFromArray(m_msgbuf, m_lenmsg)) {
        // not a valid msg header
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_INV_MSG_HDR);
        return -1;
    }

    if (hdr.msg_header().msg_type() != admin::MsgType::Result) {
        // not a command message
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_NOT_RESULT);
        return -1;
    }

    msgResult.Clear();
    if (!msgResult.ParseFromString(m_msgbuf) &&
            !msgResult.ParseFromArray(m_msgbuf, m_lenmsg)) {
        // parser command message error
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_INV_MSG_BDY);
        return -1;
    }

    return 0;
}

/*****
 * build MsgResult, and send to net.
 *****/
int CMsgRW::ReplyResult(std::string rc, std::string msg)
{
    admin::MsgResult rt;
    rt.set_allocated_msg_header(MakeMsgHdr(admin::MsgType::Result));
    rt.set_rc(rc);
    rt.set_msg(msg);

    std::string msgStr;
    if (!rt.SerializeToString(&msgStr)) {
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_PACK_FAIL);
        return -1;
    }

    if (SendPacket((char *)msgStr.c_str(), msgStr.size()) != msgStr.size()) {
        return -1;
    }
    return 0;
}

/*****
 * replay result with apdmsg
 *****/
int CMsgRW::ReplyResult(std::string rc, std::string msg, const std::map<std::string, std::string>& apdmsg)
{
    admin::MsgResult rt;
    rt.set_allocated_msg_header(MakeMsgHdr(admin::MsgType::Result));
    rt.set_rc(rc);
    rt.set_msg(msg);
    for (auto pair : apdmsg)
    {
        auto kv = rt.add_rt_pairs();
        kv->set_key(pair.first);
        kv->set_val(pair.second);
    }

    std::string msgStr;
    if (!rt.SerializeToString(&msgStr)) {
        snprintf(m_errbuf, LEN_ERR_MSG, RMSG_PACK_FAIL);
        return -1;
    }

    if (SendPacket((char *)msgStr.c_str(), msgStr.size()) != msgStr.size()) {
        return -1;
    }
    return 0;
}


/*****
 * send MsgCmd, recv MsgResult
 *****/
int CMsgRW::ExecCmdResult(const admin::MsgCmd& cmd, admin::MsgResult& result)
{
    int rc = 0;
    std::string pkg;
    if (!cmd.SerializeToString(&pkg)) {
        snprintf(m_errbuf, LEN_ERR_MSG, "serialize protobuf fail");
        return -1;
    }

    rc = SendPacket(pkg.c_str(), pkg.length());
    if (rc <= 0) {
        return -1;
    }

    rc = ReadResult(result);

    return rc;
}
