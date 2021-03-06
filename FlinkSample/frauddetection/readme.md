[toc]

使用o2k + Flink实现信用卡欺诈交易检测
=====

# 1. 环境要求：

* oracle 数据库（11g, 12c~19c)
* java 8及以上版本
* maven 
* intellij IDEA

# 2. 如何运行

## 2.1 准备oracle交易表格

```
sqlplus / as sysdba
create user o2k identified by o2k;
grant connect,resource to o2k;
conn o2k/o2k;
create table account(accountid int primary key, balance number);
```

## 2.2 启动o2k
```
docker run -it --name=o2k -p 9191:9191 -p 9092:9092 --pull always registry.cn-hangzhou.aliyuncs.com/woqutech/o2k
```

以下配置需要特别注意一下：
* 配置项1.1中列出的sql，请以dba权限在oracle中执行，这将配置o2k需要的帐号，以及查询系统表需要的权限。
* 配置项2.1: 输入将要检测的表：o2k.account，格式为owner_name.table_name
* 配置项3.1: 选择输出到kafka, bootstrap.servers可以不输入，直接在容器中启动kafka。account表的变更将写入topic: defaultapp.o2k.binlog

等o2k启动后，可以按照提示运行binlogdumpK，从kafka读取binlog并打印出来。

现在更新account表，看binlogdumpK的输出：
```
insert into account values(1,10000);
insert into account values(2,20000);
insert into account values(3,30000);
commit;
```
如果网络比较快，应该马上就能看到binlogdumpK输出了相应的binlog。

注意: binlogdumpK只是为了观察一下o2k的输出，你可以随时关掉它，这并不影响o2k和Flink程序的运行。

现在，o2k已经正常工作了。

## 2.3 启动flink程序：FraudDetectionJob
* 用IDEA打开frauddetection
* run
* 注意：如里你上面配置的不是o2k.account,而是其它schema，如bank.account，请将FraudDetectionJob.java中的topic name由defaultapp.o2k.binlog改为defaultapp.bank.binlog

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


