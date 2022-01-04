import java.util.function.Function;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;

import com.google.protobuf.ByteString;
import com.google.protobuf.InvalidProtocolBufferException;
import com.woqutech.qdecoder.packages.QDecoderPacket.Ack;
import com.woqutech.qdecoder.packages.QDecoderPacket.ClientAuth;
import com.woqutech.qdecoder.packages.QDecoderPacket.Dump;
import com.woqutech.qdecoder.packages.QDecoderPacket.HeartBeat;
import com.woqutech.qdecoder.packages.QDecoderPacket.Packet;
import com.woqutech.qdecoder.packages.QDecoderPacket.PacketType;

/**
 * connect to binlog server, demp binlog
 */
class JBinlogClient {

    private long startScn;
    private String remoteAddr;
    private Socket socket = null;
    private DataOutputStream dos = null;
    private DataInputStream dis = null;
    private boolean bConnected = false;

    public JBinlogClient(String binlogServer, long startScn) {
        remoteAddr = binlogServer;
        this.startScn = startScn;
    }

    private boolean Open() {
        if (bConnected) {
            return true;
        }
        String[] ipport = remoteAddr.split(":");
        int port = 0;
        try {
            port = Integer.valueOf(ipport[1]);
        } catch (Exception e) {
            System.out.println("invalid binlog server address: " + remoteAddr);
            return false;
        }

        try {
            socket = new Socket(ipport[0], port);
            dos = new DataOutputStream(socket.getOutputStream());
            dis = new DataInputStream(socket.getInputStream());
            bConnected = true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    private void Close() {
        if (socket != null) {
            try {
                socket.close();
                socket = null;
                bConnected = false;
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private boolean SendPkg(byte[] pkg) {
        try {
            dos.writeInt(pkg.length);
            dos.write(pkg);
            dos.flush();
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    private byte[] recvPkg() {
        try {
            int len = dis.readInt();
            byte[] data = new byte[len];
            if (dis.read(data) != len) {
                throw new IOException("read fail");
            }
            
            return data;
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        }
    }

    private boolean SendProtobufMsg(PacketType pkgType, ByteString msgBody) {
        Packet msg = Packet.newBuilder().setType(pkgType).setBody(msgBody).build();
        return SendPkg(msg.toByteArray());
    }

    private Packet RecvProtobufMsg() {
        byte[] pkg = recvPkg();
        if (pkg == null) {
            return null;
        }

        try {
            return Packet.parseFrom(pkg);
        } catch (InvalidProtocolBufferException e) {
            e.printStackTrace();
        }
        return null;
    }

    private boolean SendHeartbeat() {
        HeartBeat msg = HeartBeat.newBuilder().build();
        return SendProtobufMsg(PacketType.HEARTBEAT, msg.toByteString());
    }

    private boolean SendAck(int rc, String errMsg) {
        Ack msg = Ack.newBuilder().setErrorCode(rc).setErrorMessage(errMsg).build();
        return SendProtobufMsg(PacketType.ACK, msg.toByteString());
    }

    private boolean HandShake() {
        // recv HANDSHAKE
        Packet handShake = RecvProtobufMsg();
        if (handShake == null || handShake.getType()!=PacketType.HANDSHAKE) {
            return false;
        }

        // send AUTH
        ClientAuth auth = ClientAuth.newBuilder().build();
        if (!SendProtobufMsg(PacketType.CLIENTAUTHENTICATION, auth.toByteString())) {
            return false;
        }

        // recv ACK
        Packet ack = RecvProtobufMsg();
        if (ack == null || ack.getType() != PacketType.ACK) {
            return false;
        }
        
        return true;
    }

    public boolean DumpBinlog(Function<Packet, Integer> binlogHandler) {

        try {
            if (!Open()) {
                return false;
            }

            if (!HandShake()) {
                return false;
            }

            // send DUMP command
            String gtid = String.format("%d", startScn);
            Dump msgDump = Dump.newBuilder().setJournal("0").setPosition(0).setTimestamp(0).setGtid(gtid).build();

            if (!SendProtobufMsg(PacketType.DUMP, msgDump.toByteString())) {
                return false;
            }

            while (true) {
                Packet recvPkg = RecvProtobufMsg();
                if (recvPkg == null) {
                    return false;
                }

                if (binlogHandler.apply(recvPkg) != 0) {
                    break;
                }
            }
        }catch(Exception e) {

        }finally {
            Close();
        }

        return true;
    }
} // JBinlogClient

/**
 * dump binlog from socket <p>
 *   - recv binlog from binlog server <p>
 *   - deserialize binlog and print it <p>
 */
public class JBinlogDumpS {

    private static String binlogServer = "127.0.0.1:9191";
    private static long startScn = 0;
    private static boolean fgWait = true;

    private static void usage(Options options) {
        HelpFormatter formatter = new HelpFormatter();
        formatter.printHelp("JBinlogDumpS", options);
        System.exit(0);
    }

    public static void main(String[] args) throws Exception {
        // parse args
        Options options = new Options();

        // -h
        options.addOption("h", "help", false, "帮助信息");
        // -a 127.0.0.1:9192
        options.addOption("a", "binlog.server", true, "binlog server的地址和端口，如: 127.0.0.1:9192");
        // -S start_scn
        options.addOption("S", "start.scn", true, "指示qdecoder从该位点开始解析，默认是0");
        // -w wait_mode
        options.addOption("w", "wait", true, "消费完是否继续等待新的binlog, 1:等待, 0:不等待, 默认是1");

        CommandLineParser parser = new DefaultParser();
        CommandLine commandLine = null;
        try {
            commandLine = parser.parse(options, args);
        } catch (Exception e) {
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

        if (commandLine.hasOption("h")) {
            usage(options);
        }

        if (commandLine.hasOption("a")) {
            binlogServer = commandLine.getOptionValue("a");
        }

        if (commandLine.hasOption("S")) {
            String optVal = null;
            try {
                optVal = commandLine.getOptionValue("g");
                startScn = Long.valueOf(optVal);
            } catch (Exception e) {
                System.out.printf("invalid value for -S : %s\n", optVal);
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

        /**
         * begin dump binlog
         */
        System.out.printf("begin dump binlog from %s\n", binlogServer);
        JBinlogOut binlogOut = new JBinlogOut(System.out);

        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            /* print Statistics on exit */
            System.out.printf("\n=========== Statistics ===========\n");
            System.out.printf("total rows  : %d\n", binlogOut.Stat.TotalCount);
            System.out.printf("insert rows : %d\n", binlogOut.Stat.InsertCount);
            System.out.printf("update rows : %d\n", binlogOut.Stat.UpdateCount);
            System.out.printf("delete rows : %d\n", binlogOut.Stat.DeleteCount);
        }));

        JBinlogClient binlogRecver = new JBinlogClient(binlogServer, startScn);

        binlogRecver.DumpBinlog((Packet recvPkg) -> {
            if (recvPkg.getType() == PacketType.HEARTBEAT) {
                if (!fgWait) {
                    return -1;
                }
                return 0;
            }

            if (recvPkg.getType() != PacketType.MESSAGES) {
                System.out.println("invalid packet type received");
                return -1;
            }

            binlogOut.PrintBinlog(recvPkg.getBody().toByteArray());

            return 0;
        });

    } // method main
} // class JBinlogDumpS
