#ifndef _PkgEntryDef_H_
#define _PkgEntryDef_H_

#include "BinlogPackage.pb.h"
#include "BinlogEntry.pb.h"

// Packages message mainly used in assembler/CAssemblerServer.cpp
// packages message
namespace pb_packages = com::woqutech::binlog::packages;
typedef pb_packages::Packet   PTMG_PACKAGES;
typedef pb_packages::ClientAuth PTMG_AUTH;
typedef pb_packages::Ack PTMG_ACK;
typedef pb_packages::Dump PTMG_DUMP;
typedef pb_packages::HeartBeat PTMG_HEARTBEAT;


// Entry message mainly used in assembler/CSerializationProc.cpp
// Entry message
namespace pb_entry = com::woqutech::binlog::entry;
typedef pb_entry::Entry  PTMG_ENTRY;

typedef pb_entry::Header PTMG_HEADER;

typedef pb_entry::TransactionBegin   PTMG_TRANS_BEGIN;
typedef pb_entry::TransactionEnd   PTMG_TRANS_END;

typedef pb_entry::RowChange PTMG_ROWCHANGE;
typedef pb_entry::RowData PTMG_ROWDATA;
typedef pb_entry::Column PTMG_COLUMN;
typedef pb_entry::Pair   PTMG_PAIR;

#endif
