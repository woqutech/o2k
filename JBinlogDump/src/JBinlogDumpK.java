import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;
import java.time.Duration;

import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;

/**
 * dump binlog from kafka
 */
public class JBinlogDumpK {

    private static final String topicSufix = ".binlog.qdecoder";
    private static String appName = "defaultapp";
    private static List<String> schemas;
    private static String kafkaServer = "127.0.0.1:9092";
    private static String consumeGrouopId = "JBinlogDumpK";
    private static String autoCommit = "true";
    private static boolean fgWait = true;
    private static boolean fgExit = false;

    private  static void usage(Options options) {
        HelpFormatter formatter = new HelpFormatter();
        formatter.printHelp("JBinlogDumpK", options);
        System.exit(0);
    }

    public static void main(String[] args) throws Exception {
        // parse args
        Options options = new Options();

        // -h
        options.addOption("h","help",false,"帮助信息");
        // -b 127.0.0.1:9092
        options.addOption("b","bootstrap.servers",true,"kafka server的地址和端口，默认值是 127.0.0.1:9092");
        // -n appname
        options.addOption("n", "appname", true, "qdecoder解析日志的appname,代表了独立的一路解析,请参考assembler.conf:appname");
        // -s qdecoder,qbench
        options.addOption("s", "schemas", true, "table schemas, 读取这些schema下的表的binlog, 如： qdecoder,qbench");
        // -g groupid
        options.addOption("g", "group.id", true, "kafka 消费者组ID");
        // -c 1|0
        options.addOption("c", "auto.commit", true, "是否自动提交消费位点, 1:自动提交, 0:不提交， 默认是0");
        // -w 
        options.addOption("w", "wait", true, "消费完是否继续等待新的binlog, 1:等待, 0:不等待, 默认是1");

        CommandLineParser parser = new DefaultParser();
        CommandLine commandLine = null;
        try{
            commandLine = parser.parse(options,args);
        }catch(Exception e) {
            System.out.println(e.getMessage());
            usage(options);
        }

        String[] left = commandLine.getArgs();
        if (left.length > 0) {
            System.out.printf("invalid options:");
            for (String opt : left) {
                System.out.printf("%s ", opt);
            }
            System.out.println();
            usage(options);
        }

        if (commandLine.hasOption("h")){
            usage(options);
        }

        if (commandLine.hasOption("b")) {
            kafkaServer = commandLine.getOptionValue("b");
        } else {
            System.out.println("NOTE: default broker is " + kafkaServer);
        }

        if (commandLine.hasOption("n")) {
            appName = commandLine.getOptionValue("n");
        } else {
            System.out.println("NOTE: default appname is " + appName);
        }

        if (commandLine.hasOption("s")) {
            String multiSchemas = commandLine.getOptionValue("s");
            String[] schemasAry = multiSchemas.split(",");
            schemas = Arrays.asList(schemasAry);
        } else {
            System.out.println("must give -s");
            usage(options);
        }

        if (commandLine.hasOption("g")) {
            consumeGrouopId = commandLine.getOptionValue("g");
        }
        if (commandLine.hasOption("c")) {
            String opt = commandLine.getOptionValue("c").trim();
            opt = opt.toLowerCase();
            if (opt.equals("true") || opt.equals("t") || opt.equals("y") || opt.equals("1")) {
                autoCommit = "true";
            } else if (opt.equals("false") || opt.equals("f") || opt.equals("n") || opt.equals("0")) {
                autoCommit = "false";
            } else {
                System.out.println("invalid value for -c : " + opt);
                usage(options);
            }
        }
        if (commandLine.hasOption("w")) {
            if (commandLine.getOptionValue("w").equals("1")) {
                fgWait = true;
            } else {
                fgWait = false;
            }
        }

        Properties properties = new Properties();
        // 主机信息
        properties.put("bootstrap.servers", kafkaServer);
        // 群组id
        properties.put("group.id", consumeGrouopId);
        /**
         * 消费者是否自动提交偏移量,自动提交意味着使用该group.id只能消费一次数据。
         * 如果要多次读取一个binlog，请将自动提交设为false
         */
        properties.put("enable.auto.commit", autoCommit);
        /**
         * 自动提交偏移量的提交频率
         */
        properties.put("auto.commit.interval.ms", "1000");
        /**
         * 默认值latest. latest:在偏移量无效的情况下，消费者将从最新的记录开始读取数据
         * erliest:偏移量无效的情况下，消费者将从起始位置读取分区的记录。
         */
        properties.put("auto.offset.reset", "earliest");
        /**
         * 消费者在指定的时间内没有发送心跳给群组协调器，就被认为已经死亡， 协调器就会触发再均衡，把它的分区分配给其他消费者。
         */
        properties.put("session.timeout.ms", "30000");
        /**
         * 反序列化类
         */
        properties.put("key.deserializer", "org.apache.kafka.common.serialization.StringDeserializer");
        properties.put("value.deserializer", "org.apache.kafka.common.serialization.ByteArrayDeserializer");

        KafkaConsumer<String, byte[]> kafkaConsumer = new KafkaConsumer<>(properties);
        /**
         * 订阅主题:读取一个schema中的表的bilog
         */
        List<String> topics = new ArrayList<String>();
        for (String schema: schemas) {
            topics.add(appName + "." + schema + topicSufix);
        }
        kafkaConsumer.subscribe(topics);

        /**
         * begin dump binlog
         */
        System.out.printf("begin dump binlog from %s, %s:\n", kafkaServer, topics.toString());
        JBinlogOut binlogOut = new JBinlogOut(System.out);

        Runtime.getRuntime().addShutdownHook(new Thread(()->{
            /* print Statistics */
            System.out.printf("\n=========== Statistics ===========\n");
            System.out.printf("total rows  : %d\n", binlogOut.Stat.TotalCount);
            System.out.printf("insert rows : %d\n", binlogOut.Stat.InsertCount);
            System.out.printf("update rows : %d\n", binlogOut.Stat.UpdateCount);
            System.out.printf("delete rows : %d\n", binlogOut.Stat.DeleteCount);
        }));

        int nTimeO = 0;
        while (!fgExit) {
            /**
             * read binlog from kafka
             */
            Duration timeout = Duration.ofMillis(1000);
            ConsumerRecords<String, byte[]> records = kafkaConsumer.poll(timeout);
            if (records.count() <= 0) {
                nTimeO++;
                if (!fgWait && nTimeO>5) {
                    break;
                }
            }
            for (ConsumerRecord<String, byte[]> record : records) {
                binlogOut.PrintBinlog(record.value());
            }
        }

        // close kafka comsumer
        kafkaConsumer.close();

    } // method main
} // class JBinlogDumpK
