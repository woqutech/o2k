#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <google/protobuf/text_format.h>
#include "QDDEntry.pb.h"
#include "QDecoderProtocol.pb.h"
#include <arpa/inet.h>

#include "CBinlogKafka.h"
#include "rdkafkacpp.h"

#define MAX_FILE_NAME_LEN 256 

struct Statistics
{
    uint64_t TotalRows;
    uint64_t InsertCnt;
    uint64_t UpdateCnt;
    uint64_t DeleteCnt;
}gbStat;


typedef struct _topic_info
{
    bool waitMode;
    std::string groupIdNum;
    bool commit;
    std::string topicName;
    std::string nodeName;
    std::string schemaName;
    std::vector<std::string> tableName;
    std::string bootstrap;
    
}topic_info_t;

#define BINLOG_TOPIC_SUFIX ".binlog.qdecoder"

//使用说明
void usage(const char* szProgramName)
{
    fprintf(stderr, "Usage: %s  [-b bootStrapServer ] <-n nodeName > <-s schemaName> [-t tableName] \
[-m waitMode] [-g consumerGroup [-C Commit]]\n", szProgramName);
    // fprintf(stderr, "-o             : read file from <offset>;\n");
    fprintf(stderr, "-b             : bootstrap-server, the default is \"qmatrix-1:9092,qmatrix-2:9092,qmatrix-3:9092\"\n");
    fprintf(stderr, "-n             : the nodeName of reading, must be set\n");
    fprintf(stderr, "-s             : the schemaName of reading, must be set\n");
    fprintf(stderr, "-t             : the tableName of reading, if no set, will be print all table\n");
    fprintf(stderr, "-m             : wait mode\n");
    fprintf(stderr, "                0: Exit after obtaining the current all data\n");
    fprintf(stderr, "                1: Keep reading data until manual termination（Ctrl + c）\n");
    fprintf(stderr, "-g             : consume group, default is binlogdumpK\n");
    fprintf(stderr, "-C             : it is 1 to commit offset, other not commit\n");
    fprintf(stderr, "-r             : print raw storevalue\n");
    fprintf(stderr, "-h             : print this help message.\n");
    // fprintf(stderr, "-v             : print this help message.\n");
}

int PrintSql(const char * _buf, int _len);
int PintfArg(const topic_info_t& topic);
int GetInputArg(int argc, char *const *argv, struct _topic_info *topic);
static void StrSplit(const char * srcStr, const char* sep, std::vector<std::string>& out);

static bool bExit = false;
void sig_handler(int sig) {
    if (sig == SIGINT) {
        bExit=true;
    }
}

int main(int argc,char * argv[])
{
    int ret = 0;
    topic_info_t topic_info;
    topic_info.waitMode = 0;
    topic_info.commit = 0;
    topic_info.bootstrap = "qmatrix-1:9092,qmatrix-2:9092,qmatrix-3:9092";
    topic_info.groupIdNum = "binlogdumpK";
    char *buf = nullptr;
    int bufLen = 0;

    signal(SIGINT, sig_handler);

    GetInputArg(argc, argv, &topic_info);
    if (topic_info.nodeName.empty() || topic_info.schemaName.empty()) {
        fprintf(stderr, "nodeName and schemaName must be set \n");
        usage(argv[0]);
        exit(-1);
    }

    topic_info.topicName = topic_info.nodeName + "." + topic_info.schemaName + BINLOG_TOPIC_SUFIX;
    PintfArg(topic_info);

    CBinlogKafkaReader myTest(topic_info.bootstrap.c_str(), topic_info.topicName.c_str(), topic_info.groupIdNum);

    while (!bExit) {
        ret = myTest.Read(&buf, &bufLen, topic_info.tableName);
        if (ret) {
            if (RdKafka::ERR__TIMED_OUT != ret) {
                // not time out, stop reading
                fprintf(stderr, "read kafka err: %d\n", ret);
                break;
            } else if (topic_info.waitMode) {
                // timeout, wait msg
                continue;
            } else {
                // timeout, not wait
                break;
            }
        }

        // read OK, print binlog
        PrintSql(buf, bufLen);

        if (buf != nullptr) {
            delete buf;
            buf = nullptr;
        }
    } /* while */

    if (topic_info.commit) {
        ret = myTest.CommitOffset();
        if (ret) {
            fprintf(stderr, "commit offset error : %d\n", ret);
        }
    }

    /* print Statistics */
    printf("=========== Statistics ===========\n");
    printf("total rows  : %ld\n", gbStat.TotalRows);
    printf("insert rows : %ld\n", gbStat.InsertCnt);
    printf("update rows : %ld\n", gbStat.UpdateCnt);
    printf("delete rows : %ld\n", gbStat.DeleteCnt);

    return 0;
}

static bool printRaw = false;

int GetInputArg(int argc, char *const *argv, topic_info_t *topic)
{
    int ch = 0;
    std::string buf;
    while((ch = getopt(argc,argv,"hvz:n:s:t:b:m:g:S:E:C:T:r"))!= -1) {
        switch(ch) {
        case 'h':
            usage(argv[0]);
            exit(0);
        case 'b':
            topic->bootstrap = strndup(optarg, MAX_FILE_NAME_LEN);
            break;
        case 'n':
            topic->nodeName = strndup(optarg, MAX_FILE_NAME_LEN);
            break;   
        case 's':
            topic->schemaName = strndup(optarg, MAX_FILE_NAME_LEN);
            break;   
        case 't':
            buf = strndup(optarg, MAX_FILE_NAME_LEN);
            StrSplit(buf.c_str(), ",", topic->tableName);
            break;   
        case 'm':
            topic->waitMode = (0 == strcmp(optarg, "1")) ? true : false;
            break;
        case 'g':
            topic->groupIdNum = optarg;
            break;
        case 'C':
            topic->commit = (0 == strcmp(optarg, "1")) ? true : false;
            break;
        case 'r':
            printRaw = true;
            break;
        default:
            usage(argv[0]);
            exit(-1);
        }
    }

    return 0;
}

int PrintSql(const char * buf, int bufLen)
{
    /* no data */
    if (nullptr == buf || 0 >= bufLen) {
        return -1;
    }

    /* print entry info */
    std::string data(buf, bufLen);
    com::woqutech::qdecoder::entry::Entry row_entry;
    row_entry.ParseFromString(data);
    std::string str;
    std::string _storevalue;
    if (!printRaw) {
        _storevalue = row_entry.storevalue();
        row_entry.clear_storevalue();
    }
    google::protobuf::TextFormat::PrintToString(row_entry, &str);
    printf("\n-- binlog :\n%s", str.c_str());

    const std::string& storevalue = printRaw? row_entry.storevalue() : _storevalue;
    str.clear();

    /* print entry info */
    ::google::protobuf::Message *msg=NULL;
    if((row_entry.entrytype())==com::woqutech::qdecoder::entry::TRANSACTIONEND) {
        com::woqutech::qdecoder::entry::TransactionEnd tran_end;
        tran_end.ParseFromString(storevalue);
        google::protobuf::TextFormat::PrintToString(tran_end, &str);
    } else if((row_entry.entrytype())==com::woqutech::qdecoder::entry::TRANSACTIONBEGIN) {
        com::woqutech::qdecoder::entry::TransactionBegin tran_begin;
        tran_begin.ParseFromString(storevalue);
        google::protobuf::TextFormat::PrintToString(tran_begin, &str);
    } else if((row_entry.entrytype())==com::woqutech::qdecoder::entry::ROWDATA) {
        com::woqutech::qdecoder::entry::RowChange row_change;
        row_change.ParseFromString(storevalue);
        google::protobuf::TextFormat::PrintToString(row_change, &str);

        gbStat.TotalRows++;
        switch (row_change.eventtype()) {
            case com::woqutech::qdecoder::entry::EventType::INSERT:
                gbStat.InsertCnt++;
                break;
            case com::woqutech::qdecoder::entry::EventType::UPDATE:
                gbStat.UpdateCnt++;
                break;
            case com::woqutech::qdecoder::entry::EventType::DELETE:
                gbStat.DeleteCnt++;
                break;
            default:
                break;
        }
    } else {
        return 0;
    }
    printf("-- storevalue:\n%s", str.c_str());

    fflush(stdout);
    return 0;
}

int PintfArg(const topic_info_t& topic)
{
    fprintf(stderr, "nodeName= %s \n", topic.nodeName.c_str());
    fprintf(stderr, "schemaName= %s \n", topic.schemaName.c_str());
    for (const auto& tbName : topic.tableName) {
        fprintf(stderr, "tableName= %s \n", tbName.c_str());
    }
    fprintf(stderr, "bootstrap= %s \n", topic.bootstrap.c_str());
    fprintf(stderr, "waitMode= %d \n", topic.waitMode);
    fprintf(stderr, "groupId= %s \n", topic.groupIdNum.c_str());
    fprintf(stderr, "commit= %d \n", topic.commit);


    return 0;
}

static void StrSplit(const char * srcStr, const char* sep,
                std::vector<std::string>& out)
{
    int len1 = strlen(srcStr);
    int len2 = strlen(sep);
    if (len1<=0) {
        return;
    }
    if (len2<=0) {
        out.push_back(srcStr);
        return;
    }

    const char *end = srcStr + len1;
    while (srcStr) {
        const char *p = strstr(srcStr, sep);
        if (p) {
            std::string one(srcStr, p-srcStr);
            if (one.size() > 0) {
                out.push_back(one);
            }
            p += len2;
            if (p >= end) {
                return;
            }
        } else {
            if (srcStr < end) {
                out.push_back(srcStr);
            }
            break;
        }
        srcStr = p;
    }
    return;
}