#ifndef TOKEN_SNAPSHOT_SECTIONS_H
#define TOKEN_SNAPSHOT_SECTIONS_H

#include "token.h"
#include "snapshot.h"
#include "block_chain.h"
#include "snapshot_file.h"

namespace Token{
    class SnapshotSection{
    protected:
        Snapshot* snapshot_;

        SnapshotSection(Snapshot* snapshot):
                snapshot_(snapshot){}
    public:
        virtual ~SnapshotSection() = default;

        Snapshot* GetSnapshot() const{
            return snapshot_;
        }

        virtual bool Accept(SnapshotReader* reader) = 0;
        virtual bool Accept(SnapshotWriter* writer) = 0;
    };

    class SnapshotPrologueSection : public SnapshotSection{
    private:
        static inline BlockHeader
        ReadBlockChainHead(SnapshotFile* file){
            RawBlockHeader raw;
            if(!file->ReadMessage(raw)){
                std::stringstream ss;
                ss << "Couldn't read block chain head from snapshot file: " << file->GetFilename();
                CrashReport::GenerateAndExit(ss);
            }
            return BlockHeader(raw);
        }

        static inline void
        WriteBlockChainHead(SnapshotFile* file){
            RawBlockHeader head;
            head << BlockChain::GetHead();
            file->WriteMessage(head);
        }
    public:
        SnapshotPrologueSection(): SnapshotSection(nullptr){}
        SnapshotPrologueSection(Snapshot* snapshot): SnapshotSection(snapshot){}
        ~SnapshotPrologueSection() = default;

        bool Accept(SnapshotWriter* writer){
            SnapshotFile* file = writer->GetFile();

            uint32_t timestamp = GetCurrentTime();
            LOG(INFO) << "writing snapshot w/ timestamp: " << timestamp;

            file->WriteInt(timestamp);
            file->WriteString(Token::GetVersion());
            if(BlockChain::IsInitialized()){
                WriteBlockChainHead(file);
            }

            file->Flush();
            return true;
        }

        bool Accept(SnapshotReader* reader){
            Snapshot* snapshot = GetSnapshot();
            SnapshotFile* file = reader->GetFile();

            snapshot->timestamp_ = file->ReadInt();
            snapshot->version_ = file->ReadString();
            snapshot->head_ = ReadBlockChainHead(file);
            return true;
        }
    };
}

#endif //TOKEN_SNAPSHOT_SECTIONS_H
