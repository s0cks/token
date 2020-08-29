#ifndef TOKEN_SNAPSHOT_UNCLAIMED_TRANSACTION_POOL_H
#define TOKEN_SNAPSHOT_UNCLAIMED_TRANSACTION_POOL_H

#include "snapshot.h"
#include "snapshot_writer.h"
#include "snapshot_reader.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
    class UnclaimedTransactionPoolIndexTableWriter : public IndexTableWriter, public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolIndexTableWriter(SnapshotWriter* writer, IndexTable& table):
                IndexTableWriter(writer, table),
                UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolIndexTableWriter() = default;

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            uint256_t hash = utxo->GetHash();
            IndexReference* ref = CreateNewReference(hash);
            GetWriter()->WriteReference((*ref));
            return true;
        }
    };

    class UnclaimedTransactionPoolIndexDataTableWriter : public IndexTableDataWriter, public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolIndexDataTableWriter(SnapshotWriter* writer, IndexTable& table):
                IndexTableDataWriter(writer, table),
                UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolIndexDataTableWriter() = default;

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            uint256_t hash = utxo->GetHash();
            if(!HasReference(hash)){
                LOG(WARNING) << "cannot find reference for " << hash << " in index table";
                return false;
            }

            size_t size = utxo->GetBufferSize();
            IndexReference* ref = GetReference(hash);
            ref->SetDataPosition(GetWriter()->GetCurrentPosition());
            ref->SetSize(size);

            ByteBuffer bytes(size);
            if(!utxo->Encode(&bytes)){
                LOG(WARNING) << "cannot serialize unclaimed transaction " << hash << " to snapshot";
                return false;
            }

            WriteData(&bytes, size);
            return true;
        }
    };

    class UnclaimedTransactionPoolIndexTableLinker : public IndexTableLinker, public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolIndexTableLinker(SnapshotWriter* writer, IndexTable& table):
                IndexTableLinker(writer, table),
                UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolIndexTableLinker() = default;

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            uint256_t hash = utxo->GetHash();
            if(!HasReference(hash)){
                LOG(WARNING) << "cannot find index table reference to: " << hash;
                return false;
            }

            IndexReference* ref = GetReference(hash);
            int64_t current_pos = GetWriter()->GetCurrentPosition();
            int64_t index_pos = ref->GetIndexPosition();
            GetWriter()->SetCurrentPosition(index_pos);
            GetWriter()->WriteReference((*ref));
            GetWriter()->SetCurrentPosition(current_pos);
            return true;
        }
    };

    class SnapshotUnclaimedTransactionPoolSection : public IndexedSection{
        friend class Snapshot;
    protected:
        bool WriteIndexTable(SnapshotWriter* writer){
            LOG(INFO) << "writing " << UnclaimedTransactionPool::GetNumberOfUnclaimedTransactions() << " unclaimed transactions to snapshot: " << writer->GetFilename();
            writer->WriteLong(UnclaimedTransactionPool::GetNumberOfUnclaimedTransactions());
            LOG(INFO) << "writing unclaimed transaction pool index table to snapshot...";
            UnclaimedTransactionPoolIndexTableWriter tbl_writer(writer, index_);
            UnclaimedTransactionPool::Accept(&tbl_writer);
            return true;
        }

        bool WriteData(SnapshotWriter* writer){
            LOG(INFO) << "writing unclaimed transaction pool data to snapshot...";
            UnclaimedTransactionPoolIndexDataTableWriter data_writer(writer, index_);
            UnclaimedTransactionPool::Accept(&data_writer);
            return true;
        }

        bool LinkIndexTable(SnapshotWriter* writer){
            LOG(INFO) << "linking unclaimed transaction pool index table in snapshot....";
            UnclaimedTransactionPoolIndexTableLinker tbl_link(writer, index_);
            UnclaimedTransactionPool::Accept(&tbl_link);
            return true;
        }
    public:
        SnapshotUnclaimedTransactionPoolSection():
                IndexedSection(SnapshotSection::kUnclaimedTransactionPoolSection){}
        ~SnapshotUnclaimedTransactionPoolSection() = default;

        void operator=(const SnapshotUnclaimedTransactionPoolSection& section){
            IndexedSection::operator=(section);
        }
    };
}

#endif //TOKEN_SNAPSHOT_UNCLAIMED_TRANSACTION_POOL_H