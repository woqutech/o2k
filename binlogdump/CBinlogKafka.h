#ifndef _BINLOG_KAFKA
#define _BINLOG_KAFKA

#include <unordered_map>
#include <functional>

#include "CBinlogReader.h"
#include "rdkafkacpp.h"

/* ============================= reader =================================== */

/**
 * read binlog from kafka
 */
class CBinlogKafkaReader : public CBinlogReader {
public:
    CBinlogKafkaReader(const std::string& kafkaServer, const std::string& topicName, const std::string& groupId) {
        Init(kafkaServer, topicName, groupId    );
    }
    ~CBinlogKafkaReader();
    /**
     * read data from kafka
     * @return: 0 -success
     *          same as RdKafka::ErrorCode
     */
    int Read(char **ppMsg, int *msgLen, const std::vector<std::string>& filterKeys = {}) override;
    /**
     * Synchronous commit offset
     */
    int CommitOffset() override;

private:
    int Init(const std::string& kafkaServer, const std::string& topicName, const std::string& groupId);
    int fillOffset();

    RdKafka::Conf * m_conf = nullptr;
    RdKafka::Conf * m_tconf = nullptr;
    RdKafka::KafkaConsumer * m_consumer = nullptr;
    std::vector<RdKafka::TopicPartition*> m_logOffset;
    std::vector<RdKafka::TopicPartition*> m_readOffSet;
    
                   
};
#endif /*  _BINLOG_KAFKA */
