package spendreport;

import com.alibaba.otter.canal.protocol.CanalEntry;
import org.apache.flink.api.common.serialization.DeserializationSchema;
import org.apache.flink.api.common.serialization.SerializationSchema;
import org.apache.flink.api.common.typeinfo.TypeInformation;
import org.apache.flink.walkthrough.common.entity.Transaction;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;

public class BinlogTransactionSchema implements DeserializationSchema<Transaction>, SerializationSchema<Transaction> {
    private static final Logger LOG = LoggerFactory.getLogger(BinlogTransactionSchema.class);

    @Override
    public Transaction deserialize(byte[] bytes) throws IOException {
        if (bytes == null || bytes.length == 0) {
            // skip tombstone messages
            LOG.warn("no message body read from kafka");
            return null;
        }

        CanalEntry.Entry entry = CanalEntry.Entry.parseFrom(bytes);
        if (entry.getEntryType() != CanalEntry.EntryType.ROWDATA) {
            // not DML or DDL
            LOG.debug("not a DML or DDL entry: " + entry.getEntryType());
            return null;
        }
        if (!entry.getHeader().getTableName().equalsIgnoreCase("account")) {
            // not account table
            LOG.debug("not a entry of the table account: " + entry.getHeader().getTableName());
            return null;
        }

        CanalEntry.RowChange rowChange = CanalEntry.RowChange.parseFrom(entry.getStoreValue());
        if (rowChange.getEventType() != CanalEntry.EventType.UPDATE) {
            // only concern UPDATE balance
            LOG.debug("not UPDATE operation: " + rowChange.getEventType());
            return null;
        }
        if (rowChange.getRowDatasCount() <= 0) {
            // no row data
            LOG.debug("no RowData in RowChange");
            return null;
        }

        long executeTimeMs = entry.getHeader().getExecuteTime();
        CanalEntry.RowData rowData = rowChange.getRowDatas(0);

        return rowData2Transaction(rowData, executeTimeMs);
    }

    private Transaction rowData2Transaction(CanalEntry.RowData rowData, long executeTimeMs) {
        class MyRow {
            public long accountId;
            public double balance;

            public MyRow() {
                accountId = -1;
                balance = -1;
            }
        };
        MyRow oldRow = new MyRow();
        MyRow newRow = new MyRow();

        for (CanalEntry.Column col : rowData.getBeforeColumnsList()) {
            if (col.getName().equalsIgnoreCase("accountid")) {
                oldRow.accountId = Long.parseLong(col.getValue());
            } else if (col.getName().equalsIgnoreCase("balance")) {
                oldRow.balance = Double.parseDouble(col.getValue());
            }
        }

        for (CanalEntry.Column col : rowData.getAfterColumnsList()) {
            if (col.getName().equalsIgnoreCase("accountid")) {
                newRow.accountId = Long.parseLong(col.getValue());
            } else if (col.getName().equalsIgnoreCase("balance")) {
                newRow.balance = Double.parseDouble(col.getValue());
            }
        }

        if (newRow.accountId < 0 || newRow.balance < 0 || oldRow.balance < 0) {
            LOG.warn("invalid accountid or balance:" + newRow.accountId + ":" + newRow.balance);
            return null;
        }

        LOG.debug("account-" + newRow.accountId + ": " + oldRow.balance + " -> " + newRow.balance + ", " + (newRow.balance-oldRow.balance));

        return new Transaction(newRow.accountId, executeTimeMs, Math.abs(newRow.balance-oldRow.balance));
    }

    @Override
    public boolean isEndOfStream(Transaction transaction) {
        return false;
    }

    @Override
    public byte[] serialize(Transaction transaction) {
        return new byte[0];
    }

    @Override
    public TypeInformation<Transaction> getProducedType() {
        return TypeInformation.of(Transaction.class);
    }
}
