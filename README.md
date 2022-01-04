# QDecoder 工具

## 1. java binlogdump范例：读取并打印binlog

QDecoder从oracle解析归档日志文件和在线日志文件，生成binlog，并写入kafka，或文件，或socket。
JBinlogDump工具分别从kafka和binlog server读取binlog:

* JBinlogDumpK: 从kafka读取binlog，反序列化成Java object，打印到控制台。
* JBinlogDumpS: 从socket(binlog server)读取binlog，反序列化成Java object，打印到控制台。

### 1.1 编译

要求jdk版本： 不低于java 8

```
cd JBinlogDump

javac -cp .:lib/commons-cli-1.5.0.jar:lib/protobuf-3.6.1.jar:lib/kafka-clients-3.0.0.jar:lib/log4j-1.2.17.jar:lib/slf4j-api-1.7.30.jar:lib/slf4j-log4j12-1.7.30.jar:lib/qdecoder-proto.jar src/*.java
```

### 1.2 运行JBinlogDumpK: 从kafka读取binlog并打印出来

```
cd JBinlogDump

java  -cp .:lib/commons-cli-1.5.0.jar:lib/protobuf-3.6.1.jar:lib/kafka-clients-3.0.0.jar:lib/log4j-1.2.17.jar:lib/slf4j-api-1.7.30.jar:lib/slf4j-log4j12-1.7.30.jar:lib/qdecoder-proto.jar:src JBinlogDumpK -b 127.0.0.1:9092 -s qdecoder,schema2
```
参数说明:

 -h 显示帮助信息
 
 -b kafka.bootstrap.server, 默认是127.0.0.1：9092
 
 -s 指定oracle schema，多个schema用逗号分隔。QDecoder会将每一个schema下的所有table的binlog写入一个kafka topic，所以，必须指定要读取哪些schema的binlog。


### 1.3 运行JBinlogDumpS: 从socket读取binlog并打印出来

```
cd JBinlogDump

java  -cp .:lib/commons-cli-1.5.0.jar:lib/protobuf-3.6.1.jar:lib/kafka-clients-3.0.0.jar:lib/log4j-1.2.17.jar:lib/slf4j-api-1.7.30.jar:lib/slf4j-log4j12-1.7.30.jar:lib/qdecoder-proto.jar:src JBinlogDumpS -a 127.0.0.1:9191
```

参数说明:

  -h 显示帮助信息
  
  -a binlog server的地址和端口号，默认是127.0.0.1:9191。当QDecoder输出到socket时，默认会绑定9191端口。


