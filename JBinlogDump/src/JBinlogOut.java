
import java.io.PrintStream;

import com.google.protobuf.ByteString;
import com.google.protobuf.TextFormat;

import com.woqutech.qdecoder.entry.QDDEntry.Entry;
import com.woqutech.qdecoder.entry.QDDEntry.RowChange;
import com.woqutech.qdecoder.entry.QDDEntry.TransactionEnd;
import com.woqutech.qdecoder.entry.QDDEntry.TransactionBegin;
import com.woqutech.qdecoder.entry.QDDEntry.EventType;

/**
 * deserialize binlog and print
 */
public class JBinlogOut {

    private PrintStream out = null;

    public JBinlogOut(PrintStream outStream) {
        out = outStream;
        Stat = new Statistics();
    }

    public class Statistics {
        public long TotalCount = 0;
        public long InsertCount = 0;
        public long UpdateCount = 0;
        public long DeleteCount = 0;
        public long DMLCount = 0;
        public long DDLCount = 0;
    };

    public Statistics Stat;

    public void PrintBinlog(byte[] binlog) {
        try {
            Entry entry = Entry.parseFrom(binlog);

            // print entry header and type
            out.println("--binlog:");
            out.println(TextFormat.printToString(entry.toBuilder().clearStoreValue().build()));

            // print store value of entry
            out.println("--storevalue:");
            ByteString storeValue = entry.getStoreValue();
            String body;
            switch (entry.getEntryType()) {
            case TRANSACTIONBEGIN:
                TransactionBegin begin = TransactionBegin.parseFrom(storeValue);
                body = TextFormat.printToString(begin);
                break;
            case TRANSACTIONEND:
                TransactionEnd end = TransactionEnd.parseFrom(storeValue);
                body = TextFormat.printToString(end);
                break;
            case ROWDATA:
                RowChange row = RowChange.parseFrom(storeValue);
                body = TextFormat.printToString(row);

                Stat.TotalCount++;
                if (row.getIsDdl()) {
                    Stat.DDLCount++;
                } else {
                    Stat.DMLCount++;
                    switch (row.getEventType()) {
                    case INSERT:
                        Stat.InsertCount++;
                        break;
                    case UPDATE:
                        Stat.UpdateCount++;
                        break;
                    case DELETE:
                        Stat.DeleteCount++;
                        break;
                    default:
                        break;
                    }
                }
                break;
            default:
                body = "{ !!! unknow binlog !!! }";
                break;
            }

            out.println(body);

        } catch (Exception e) {
            System.out.println(e.getMessage());
        }
    } // PrintBinlog
} // class JBinlogOut
