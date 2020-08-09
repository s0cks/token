#include "token.h"
#include "snapshot.h"
#include "block_chain.h"

namespace Token{
    bool Snapshot::WriteSnapshot(Snapshot* snapshot){
        SnapshotWriter writer;
        return writer.WriteSnapshot();
    }

    bool Snapshot::WriteNewSnapshot(){
        Snapshot snapshot;
        return WriteSnapshot(&snapshot);
    }

    Snapshot* Snapshot::ReadSnapshot(const std::string& filename){
        SnapshotReader reader(filename);
        return reader.ReadSnapshot();
    }


#define CHECK_FILE_POINTER \
    if(GetFilePointer() == NULL){ \
        LOG(WARNING) << "snapshot file " << GetFilename() << " is not opened"; \
        return; \
    }
#define CHECK_FILE_STATUS(Result, Error) \
    if((Result) != 0){ \
        LOG(WARNING) << (Error); \
        return; \
    }

    SnapshotFile::~SnapshotFile(){
        if(GetFilePointer() != NULL) Close();
    }

    void SnapshotFile::WriteBytes(uint8_t* bytes, size_t size){
        CHECK_FILE_POINTER;
        LOG(INFO) << "writing " << sizeof(bytes) << "/" << size << " bytes";

        size_t written_bytes;
        if((written_bytes = fwrite(bytes, sizeof(uint8_t), size, GetFilePointer())) != size){
            LOG(WARNING) << "could only write " << written_bytes << "/" << size << " bytes to snapshot file: " << GetFilename();
        }
        fflush(GetFilePointer());
    }

    void SnapshotFile::WriteInt(uint32_t value){
        CHECK_FILE_POINTER;

        size_t written_bytes;
        if((written_bytes = fwrite(&value, sizeof(uint32_t), 1, GetFilePointer())) != 1)
            LOG(WARNING) << "could only write " << written_bytes << "/" << sizeof(uint32_t) << " bytes to snapshot file: " << GetFilename();

        int result = fflush(GetFilePointer());
        CHECK_FILE_STATUS(result, "couldn't flush snapshot file");
    }

    void SnapshotFile::WriteString(const std::string& value){
        WriteInt(value.length());
        WriteBytes((uint8_t*)value.data(), value.length());
    }

    void SnapshotFile::WriteMessage(google::protobuf::Message& msg){
        uint32_t size = msg.ByteSizeLong();
        WriteInt(size);

        uint8_t data[size];
        if(!msg.SerializeToArray(data, size)){
            LOG(WARNING) << "couldn't serialize message to snapshot file: " << GetFilename();
            return;
        }
        WriteBytes(data, size);
    }

    bool SnapshotFile::ReadBytes(uint8_t* bytes, size_t size){
        return fread(bytes, sizeof(uint8_t), size, GetFilePointer()) == size;
    }

    bool SnapshotFile::ReadMessage(google::protobuf::Message& msg){
        uint32_t num_bytes = ReadInt();
        uint8_t bytes[num_bytes];
        if(!ReadBytes(bytes, num_bytes)) return false;
        return msg.ParseFromArray(bytes, num_bytes);
    }

    uint8_t SnapshotFile::ReadByte(){
        uint8_t bytes[1];
        if(!ReadBytes(bytes, 1)){
            std::stringstream ss;
            ss << "Couldn't read byte from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return bytes[0];
    }

    uint32_t SnapshotFile::ReadInt(){
        uint8_t bytes[4];
        if(!ReadBytes(bytes, 4)){
            std::stringstream ss;
            ss << "Couldn't read int from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return (*(uint32_t*)bytes);
    }

    std::string SnapshotFile::ReadString(){
        uint32_t size = ReadInt();
        uint8_t bytes[size];
        if(!ReadBytes(bytes, size)){
            std::stringstream ss;
            ss << "Couldn't read string of size " << size << " from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return std::string((char*)bytes, size);
    }

    void SnapshotFile::Flush(){
        CHECK_FILE_POINTER;
        int err;
        if((err = fflush(GetFilePointer())) != 0) LOG(WARNING) << "couldn't flush snapshot file " << GetFilename() << ": " << strerror(err);
    }

    void SnapshotFile::Close(){
        CHECK_FILE_POINTER;
        int err;
        if((err = fclose(GetFilePointer())) != 0) LOG(WARNING) << "couldn't close snapshot file " << GetFilename() << ": " << strerror(err);
    }

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

    bool SnapshotWriter::WriteSnapshot(){
        SnapshotPrologueSection prologue;
        if(!prologue.Accept(this)){
            LOG(WARNING) << "couldn't write prologue section to snapshot: " << GetFile()->GetFilename();
            return false;
        }

        GetFile()->Flush();
        GetFile()->Close();
        return true;
    }

    Snapshot* SnapshotReader::ReadSnapshot(){
        Snapshot* snapshot = new Snapshot();

        SnapshotPrologueSection prologue(snapshot);
        if(!prologue.Accept(this)){
            LOG(WARNING) << "couldn't read prologue section from snapshot: " << GetFile()->GetFilename();
            delete snapshot;
            return nullptr;
        }

        return snapshot;
    }
}