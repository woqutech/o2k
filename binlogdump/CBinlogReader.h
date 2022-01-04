#ifndef _BINLOG_READ
#define _BINLOG_READ

#include <string>
#include<vector>

class CBinlogReader {
public:
    virtual ~CBinlogReader() {}
    virtual int Read(char **ppMsg, int *msgLen, const std::vector<std::string>& filterKeys) = 0;
    virtual int CommitOffset() = 0;
};


#endif /* _BINLOG_WRITER */
