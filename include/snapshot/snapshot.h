#ifndef TOKEN_SNAPSHOT_H
#define TOKEN_SNAPSHOT_H

#include <memory>
#include "block.h"
#include "section.h"
#include "job/job.h"
#include "utils/file_reader.h"
#include "utils/file_writer.h"

namespace Token{
  class Snapshot;
  typedef std::shared_ptr<Snapshot> SnapshotPtr;

  class SnapshotVisitor;
  class Snapshot{
    friend class SnapshotWriter;
    friend class SnapshotReader;
    friend class SnapshotInspector;
   private:
    std::string filename_;
    SnapshotPrologueSection prologue_;
    SnapshotBlockChainSection blocks_;

    static inline void
    CheckSnapshotDirectory(){
      std::string filename = GetSnapshotDirectory();
      if(!FileExists(filename)){
        if(!CreateDirectory(filename))
          LOG(WARNING) << "couldn't create snapshots directory: " << filename;
      }
    }
   public:
    Snapshot(const std::string& filename):
      filename_(filename),
      prologue_(),
      blocks_(){}
    Snapshot(const std::string& filename,
      const SnapshotPrologueSection& prologue,
      const SnapshotBlockChainSection& blocks):
      filename_(filename),
      prologue_(prologue),
      blocks_(blocks){}
    ~Snapshot() = default;

    std::string GetFilename() const{
      return filename_;
    }

    SnapshotPrologueSection& GetPrologue(){
      return prologue_;
    }

    SnapshotBlockChainSection& GetBlockChain(){
      return blocks_;
    }

    Timestamp GetTimestamp() const{
      return prologue_.GetTimestamp();
    }

    Version GetVersion() const{
      return prologue_.GetVersion();
    }

    Hash GetHead() const{
      return blocks_.GetHead();
    }

    static bool WriteNewSnapshot();
    static SnapshotPtr ReadSnapshot(const std::string& filename);

    static SnapshotPtr NewInstance(const std::string& filename){
      return std::make_shared<Snapshot>(filename);
    }

    static inline std::string
    GetSnapshotDirectory(){
      return (TOKEN_BLOCKCHAIN_HOME + "/snapshots");
    }
  };

  class SnapshotVisitor{
   protected:
    SnapshotVisitor() = default;
   public:
    virtual ~SnapshotVisitor() = default;
    virtual bool VisitStart(Snapshot* snapshot){ return true; }
    virtual bool Visit(Block* blk) = 0;
    virtual bool VisitEnd(Snapshot* snapshot){ return true; }
  };

  class SnapshotPrinter : Printer{
   public:
    SnapshotPrinter(Printer* parent):
      Printer(parent){}
    SnapshotPrinter(const google::LogSeverity& severity = google::INFO, const long& flags = Printer::kFlagNone):
      Printer(severity, flags){}
    ~SnapshotPrinter() = default;

    bool Print(const SnapshotPtr& snapshot){
      LOG_AT_LEVEL(GetSeverity()) << "Filename: " << snapshot->GetFilename();
      LOG_AT_LEVEL(GetSeverity()) << "Created: " << GetTimestampFormattedReadable(snapshot->GetTimestamp());
      LOG_AT_LEVEL(GetSeverity()) << "Version: " << snapshot->GetVersion();
      LOG_AT_LEVEL(GetSeverity()) << "Head: " << snapshot->GetHead();
      return true;
    }

    bool operator()(const SnapshotPtr& snapshot){
      return Print(snapshot);
    }
  };

  class SnapshotWriter : public BinaryFileWriter{
   private:
    static inline std::string
    GetNewSnapshotFilename(){
      std::stringstream filename;
      filename << Snapshot::GetSnapshotDirectory();
      filename << "/snapshot-" << GetTimestampFormattedFileSafe(GetCurrentTimestamp()) << ".dat";
      return filename.str();
    }

    inline void
    WriteHeader(const SnapshotSectionHeader& header){
      WriteUnsignedLong(header);
    }

    template<class T>
    bool WriteSection(const T& section){
      WriteHeader(section.GetSectionHeader());
      section.Write(this);
      return true;
    }
   public:
    SnapshotWriter(const std::string& filename = GetNewSnapshotFilename()):
      BinaryFileWriter(filename){}
    ~SnapshotWriter() = default;

    bool WriteSnapshot(){
      SnapshotPtr snapshot = Snapshot::NewInstance(GetFilename());
      LOG(INFO) << "writing new snapshot...";
      WriteSection(snapshot->GetPrologue());
      WriteSection(snapshot->GetBlockChain());
      LOG(INFO) << "snapshot written to: " << GetFilename();
      return true;
    }
  };

  class SnapshotReader : public BinaryFileReader{
    friend class Snapshot;
   public:
    SnapshotReader(const std::string& filename):
      BinaryFileReader(filename){}
    ~SnapshotReader() = default;

    SnapshotPtr ReadSnapshot(){
      int64_t start = GetCurrentPosition();
      if(!SnapshotSection::IsPrologueSection(ReadUnsignedLong())){
        LOG(WARNING) << "cannot read snapshot prologue section @" << start << " in: " << GetFilename();
        return SnapshotPtr(nullptr);
      }
      SnapshotPrologueSection prologue(this);

      start = GetCurrentPosition();
      if(!SnapshotSection::IsBlockChainSection(ReadUnsignedLong())){
        LOG(WARNING) << "cannot read snapshot block chain section @" << start << " in: " << GetFilename();
        return SnapshotPtr(nullptr);
      }
      SnapshotBlockChainSection blocks(this);
      return std::make_shared<Snapshot>(GetFilename(), prologue, blocks);
    }
  };

  class SnapshotJob : public Job{
   protected:
    JobResult DoWork(){
      SnapshotWriter writer;
      if(!writer.WriteSnapshot()){
        std::stringstream ss;
        ss << "Couldn't write snapshot to: " << writer.GetFilename();
        return Failed(ss);
      }
      return Success("done.");
    }
   public:
    SnapshotJob(Job* parent):
      Job(parent, "CreateSnapshot"){}
    SnapshotJob():
      SnapshotJob(nullptr){}
    ~SnapshotJob() = default;
  };
}

#endif //TOKEN_SNAPSHOT_H
