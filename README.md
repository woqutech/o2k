# o2k 工具

o2k是一款高性能、免费的oracle数据订阅同步软件，支持从Oracle数据库读取在线日志文件和归档日志文件，解析出DDL和DML操作，输出binlog到kafka供下游应用程序进行数据订阅。

本软件仓库提供了Java、C++版本的读取o2k输出的示例代码JBinlogDumpK/JBinlogDumpS/binlogdumpK；提供了动态查询和修改o2k状态及配置的工具qdcadm；提供了o2k跟Flink结合的示例程序FlinkSample/frauddetection.

## 1. java binlogdump范例：读取并打印binlog

o2k从oracle解析归档日志文件和在线日志文件，生成binlog，并写入kafka，或文件，或socket。
JBinlogDump工具分别从kafka和binlog server读取binlog:

* JBinlogDumpK: 从kafka读取binlog，反序列化成Java object，打印到控制台。
* JBinlogDumpS: 从socket(binlog server)读取binlog，反序列化成Java object，打印到控制台。

### 1.1 编译

要求jdk版本： 不低于java 8

```
cd JBinlogDump

javac -cp .:lib/commons-cli-1.5.0.jar:lib/protobuf-3.6.1.jar:lib/kafka-clients-3.0.0.jar:lib/log4j-1.2.17.jar:lib/slf4j-api-1.7.30.jar:lib/slf4j-log4j12-1.7.30.jar:lib/binlog-serializer.jar src/*.java
```

### 1.2 运行JBinlogDumpK: 从kafka读取binlog并打印出来

```
cd JBinlogDump

java  -cp .:lib/commons-cli-1.5.0.jar:lib/protobuf-3.6.1.jar:lib/kafka-clients-3.0.0.jar:lib/log4j-1.2.17.jar:lib/slf4j-api-1.7.30.jar:lib/slf4j-log4j12-1.7.30.jar:lib/binlog-serializer.jar:src JBinlogDumpK -b 127.0.0.1:9092 -s schema1,schema2
```
参数说明:

 -h 显示帮助信息
 
 -b kafka.bootstrap.server, 默认是127.0.0.1：9092
 
 -s 指定oracle schema，多个schema用逗号分隔。o2k会将每一个schema下的所有table的binlog写入一个kafka topic，所以，必须指定要读取哪些schema的binlog。


### 1.3 运行JBinlogDumpS: 从socket读取binlog并打印出来

```
cd JBinlogDump

java  -cp .:lib/commons-cli-1.5.0.jar:lib/protobuf-3.6.1.jar:lib/kafka-clients-3.0.0.jar:lib/log4j-1.2.17.jar:lib/slf4j-api-1.7.30.jar:lib/slf4j-log4j12-1.7.30.jar:lib/binlog-serializer.jar:src JBinlogDumpS -a 127.0.0.1:9191
```

参数说明:

  -h 显示帮助信息
  
  -a binlog server的地址和端口号，默认是127.0.0.1:9191。当o2k输出到socket时，默认会绑定9191端口。


## 2. C++ binlogdump范例：读取并打印binlog

### 2.1 编译

#### 2.1.1 环境要求

* gcc版本: 不低于4.8
* protobuf lib: 3.6
* protobuf dev: 3.6
* protobuf compile: 3.6
* librdkafka: 

安装protobuf:

centos:
```
yum install protobuf protobuf-compiler protobuf-devel
```

ubuntu:
```
sudo apt install libprotobuf protobuf-compiler libprotobuf-dev
```

安装rdkafka:

[https://github.com/edenhill/librdkafka]


#### 2.1.2 生成protobuf序列化代码

```
cp proto/* binlogdump
cd binlogdump

protoc --cpp_out=. *.proto
```

#### 2.1.3 编译binlogdumpK

```
cd binlogdump

g++ -o binlogdumpK *.cpp *.cc -I. -I/usr/local/include/librdkafka -lprotobuf -lrdkafka++
```

## 2.2 运行binlogdumpK

```
./binlogdumpK -b 127.0.0.1:9092 -n defaultapp -s qbench
```

# 3. qdcadm: o2k交互管理工具

## 3.1 编译

### 3.1.1 环境要求

同binlogdump

### 3.1.2 编译qdcadm

```
cd qdcadm

g++ -o qdcadm *.cpp *.cc -I. -lprotobuf -lpthread
```

## 3.2 运行qdcadm

```
cd qdcadm
./qdcadm 127.0.0.1:9193 woqutech list
./qdcadm 127.0.0.1:9193 woqutech get_conf binlog.output.dest
```

# 4.联系方式
微信群：扫描下方二维码，添加企业微信，发送“o2k社区交流”后，拉入群聊

![o2k拉群二维码](https://user-images.githubusercontent.com/96899373/154804920-449767b0-91e3-4945-9fa9-c031e1209d1a.png)

