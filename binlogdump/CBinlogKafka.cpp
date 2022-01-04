#include <functional>

#include "CBinlogKafka.h"
#include "utmisc.h"

class ExampleEventCb : public RdKafka::EventCb {
public:
    void event_cb (RdKafka::Event &event) {
        switch (event.type())
        {
        case RdKafka::Event::EVENT_ERROR:
            printf("kafka event error: %s, %s", event.str().c_str(),
                   RdKafka::err2str(event.err()).c_str());
            if (event.err() == RdKafka::ERR__ALL_BROKERS_DOWN) {
                // no brokers accessible
            }
            break;

        case RdKafka::Event::EVENT_STATS:
            break;

        case RdKafka::Event::EVENT_LOG:
            break;

        default:
            break;
        }
    }
};

class ExampleConsumeCb : public RdKafka::ConsumeCb {
public:
    void consume_cb (RdKafka::Message &msg, void *opaque) {

    }
};


/* =============================== reader ==================================== */

CBinlogKafkaReader::~CBinlogKafkaReader()
{
    if (m_conf) delete m_conf;
    if (m_tconf) delete m_tconf;
    if (m_consumer) {
        m_consumer->close();
        delete m_consumer;
    }
    for (auto tp : m_readOffSet) {
        delete tp;
    }
    for (auto tp : m_logOffset) {
        delete tp;
    }
    RdKafka::wait_destroyed(5000);
}

int CBinlogKafkaReader::Init(const std::string& kafkaServer, const std::string& topicName, const std::string& groupId)
{
    std::string errstr;
    std::string topic_str = topicName;
    std::vector<std::string> topics = {topic_str};

    RdKafka::Conf *conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    RdKafka::Conf *tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
    m_conf = conf;
    m_tconf = tconf;

    if (conf->set("group.id", groupId, errstr) != RdKafka::Conf::CONF_OK) {
        printf("kafka: set group.id: %s", errstr.c_str());
        return -1;
    }

    conf->set("enable.auto.commit", "false", errstr);
    conf->set("message.max.bytes", "10240000", errstr);
    conf->set("max.partition.fetch.bytes", "1024000", errstr);  

    conf->set("bootstrap.servers", kafkaServer, errstr);

    ExampleConsumeCb *ex_consume_cb = new ExampleConsumeCb();
    conf->set("consume_cb", ex_consume_cb, errstr);

    ExampleEventCb *ex_event_cb = new ExampleEventCb();
    conf->set("event_cb", ex_event_cb, errstr);

    tconf->set("auto.offset.reset", "earliest", errstr);
    conf->set("default_topic_conf", tconf, errstr);

    RdKafka::KafkaConsumer *consumer = RdKafka::KafkaConsumer::create(conf, errstr);
    if (!consumer) {
        printf("kafka: create consumer fail: %s", errstr.c_str());
        abort();
        return -1;
    }
    printf("kafka: create consumer OK: %s, for %s,%s, url is %s",
           consumer->name().c_str(), topicName.c_str(), groupId.c_str(), kafkaServer.c_str());
    m_consumer = consumer;

    RdKafka::ErrorCode err = consumer->subscribe(topics);
    if (err) {
        printf("kafka: subscribe fail: %s", RdKafka::err2str(err).c_str());
        abort();
        return -1;
    }

    return 0;
}

int CBinlogKafkaReader::fillOffset()
{
    if (m_logOffset.empty()) {
        auto errcode = m_consumer->assignment(m_readOffSet);
        if (errcode != RdKafka::ERR_NO_ERROR) {
            printf("kafka: get read offset fail: %s", RdKafka::err2str(errcode).c_str());
            return -1;
        }

        errcode = m_consumer->assignment(m_logOffset);
        if (errcode != RdKafka::ERR_NO_ERROR) {
            printf( "kafka: get log offset fail: %s", RdKafka::err2str(errcode).c_str());
            return -1;
        }

        for (auto tp : m_logOffset) {
            int64_t low, high;
            errcode = m_consumer->query_watermark_offsets(tp->topic(), tp->partition(), &low, &high, -1);
            if (errcode != RdKafka::ERR_NO_ERROR) {
                printf( "kafka: get watermark offset fail: %s", RdKafka::err2str(errcode).c_str());
                return -1;
            }
            tp->set_offset(high);
            printf( "kafka: get watermark offset OK: %s,%2d: %2d",
                   tp->topic().c_str(), tp->partition(), high);
        }
    }
    return 0;
}

int CBinlogKafkaReader::Read(char **ppMsg, int *msgLen, const std::vector<std::string>& filterKeys)
{
    std::string key;
    int readEnd;
    RdKafka::ErrorCode errcode;

    if (m_logOffset.empty()) {
        fillOffset();
    }

    *msgLen = 0;

    while (true) {
        RdKafka::Message *kmsg = m_consumer->consume(5000);
        MyDefer freeMsg([kmsg](){ delete kmsg; });
        
        switch (kmsg->err()) {
        case RdKafka::ERR__TIMED_OUT:
            
            // PrtLog(INFOLOG, "kafka: read msg timeout: %s", kmsg->errstr().c_str());
            // m_logOffset.empty() stand for do not getted partition，and there will be continued obtain
            if (m_logOffset.empty()) {
                fillOffset();
            } else {
                // get read position
                errcode = m_consumer->position(m_readOffSet);
                if (errcode != RdKafka::ERR_NO_ERROR) {
                    printf( "kafka: get current position fail: %s",
                           RdKafka::err2str(errcode).c_str());
                    return RdKafka::ERR__FAIL;
                }

                // check read over
                readEnd = 1;
                for (int i = 0; i < m_logOffset.size(); i++) {
                    if (m_readOffSet[i]->offset() <= -1001) {
                        continue; // no msg in this partition
                    }
                    if (m_readOffSet[i]->offset() < m_logOffset[i]->offset()) {
                        printf( "kafka: read to %d,%d, log to %d,%d",
                               m_readOffSet[i]->partition(), m_readOffSet[i]->offset(),
                               m_logOffset[i]->partition(), m_logOffset[i]->offset());
                        readEnd = 0;
                        break;
                    }
                }
                if (readEnd == 1) {
                    return RdKafka::ERR__TIMED_OUT;
                }
            }
            break;

         case RdKafka::ERR_NO_ERROR:
            if (m_logOffset.empty()) {
                fillOffset();
            }

            // read message
            if (!kmsg->key()) {
                printf( "kafka: no key in msg, len %d", kmsg->len());
                return RdKafka::ERR__FAIL;
            }

            // check filter
            if (!filterKeys.empty()) {
                bool bFound = false;
                for (const auto& filter : filterKeys) {
                    if (filter == *kmsg->key()) {
                        bFound = true;
                        break;
                    }
                }
                if (!bFound) {
                    continue;
                }
            }

            *msgLen = kmsg->len();
            *ppMsg = new char[*msgLen];
            memcpy((void *)*ppMsg, kmsg->payload(), *msgLen);

            return RdKafka::ERR_NO_ERROR;
            break;

        case RdKafka::ERR__PARTITION_EOF:
            /* Last message */
            return RdKafka::ERR__PARTITION_EOF;
            break;

        case RdKafka::ERR__UNKNOWN_TOPIC:
        case RdKafka::ERR__UNKNOWN_PARTITION:
            printf( "kafka: read msg: %s", kmsg->errstr().c_str());
            return RdKafka::ERR__UNKNOWN_PARTITION;
            break;

        default:
            /* Errors */
            //printf("get msg : %d\r\n", kmsg->err());
            printf( "kafka: read msg: %s", kmsg->errstr().c_str());
            return RdKafka::ERR__FAIL;

        } /* switch */
    } /* while */


    return 1;
}

int CBinlogKafkaReader::CommitOffset()
{
    return m_consumer->commitSync();
}
