[toc]

使用QDecoder + Flink实现信用卡欺诈交易检测
=====

# 1. 环境要求：

* oracle 数据库（11g, 12c~19c)
* java 8及以上版本
* maven 
* intellij IDEA

# 2. 如何运行

## 2.1 准备oracle交易表格

```
conn YOUR_ORACLE_USER_NAME/your_oracle_password
create table account(accountid int primary key, balance number);
```

## 2.2 启动QDecoder
```
docker run -it --name=qdecoder -p 9191:9191 -p 9092:9092 --pull always registry.cn-hangzhou.aliyuncs.com/woqutech/qdecoder
```

以下配置需要特别注意一下：
* 配置项1.1中列出的sql，请以dba权限在oracle中执行，这将配置QDecoder需要的帐号，以及查询系统表需要的权限。
* 配置项2.1: 输入将要检测的表：YOUR_ORACLE_USER_NAME.account，格式为owner_name.table_name
* 配置项3.1: 选择输出到kafka, bootstrap.servers可以不输入，直接在容器中启动kafka。account表的变更将写入topic: defaultapp.YOUR_ORACLE_USER_NAME.binlog.qdecoder

等QDecoder启动后，可以按照提示运行binlogdumpK，从kafka读取binlog并打印出来。

现在更新account表，看binlogdumpK的输出：
```
insert into account values(1,10000);
insert into account values(2,20000);
insert into account values(3,30000);
commit;
```
如果网络比较快，应该马上就能看到binlogdumpK输出了相应的binlog。

注意: binlogdumpK只是为了观察一下QDecoder的输出，你可以随时关掉它，这并不影响QDecoder和Flink程序的运行。

现在，QDecoder已经正常工作了。

## 2.3 启动flink程序：FraudDetectionJob
* 用IDEA打开frauddetection
* run

## 2.4 更新account表的balance字段，观察FraudDetectionJob的输出

```
update account set balance = balance - 0.1 where accountid = 1;
commit;
update account set balance = balance - 100 where accountid = 1;
commit;
update account set balance = balance - 0.2 where accountid = 2;
commit;
update account set balance = balance - 501 where accountid = 1;
commit;
update account set balance = balance - 200 where accountid = 2;
commit;
```
在一分钟内，account-1先出现了小于0.5的变更，后面又出现了大于500的变更，则识别为欺诈事务。
执行完上述SQL，Flink程序会立即输出：
```
21:11:20,107 INFO  org.apache.flink.walkthrough.common.sink.AlertSink           [] - Alert{id=1}
```
表示accountid=1的帐号检测到欺诈交易。


