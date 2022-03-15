# generate cpp code

```
protoc --cpp_out=. *.proto
```

# generate java code

```
protoc --java_out=. *.proto

javac -cp .:../JBinlogDump/lib/commons-cli-1.5.0.jar:../JBinlogDump/lib/protobuf-3.6.1.jar:../JBinlogDump/lib/kafka-clients-3.0.0.jar:../JBinlogDump/lib/log4j-1.2.17.jar:../JBinlogDump/lib/slf4j-api-1.7.30.jar:../JBinlogDump/lib/slf4j-log4j12-1.7.30.jar com/woqutech/admin/AdminMessage.java 

javac -cp .:../JBinlogDump/lib/commons-cli-1.5.0.jar:../JBinlogDump/lib/protobuf-3.6.1.jar:../JBinlogDump/lib/kafka-clients-3.0.0.jar:../JBinlogDump/lib/log4j-1.2.17.jar:../JBinlogDump/lib/slf4j-api-1.7.30.jar:../JBinlogDump/lib/slf4j-log4j12-1.7.30.jar com/woqutech/binlog/entry/BinlogEntry.java

javac -cp .:../JBinlogDump/lib/commons-cli-1.5.0.jar:../JBinlogDump/lib/protobuf-3.6.1.jar:../JBinlogDump/lib/kafka-clients-3.0.0.jar:../JBinlogDump/lib/log4j-1.2.17.jar:../JBinlogDump/lib/slf4j-api-1.7.30.jar:../JBinlogDump/lib/slf4j-log4j12-1.7.30.jar com/woqutech/binlog/packages/BinlogPacket.java 

jar --create --file binlog-serializer.jar com

```