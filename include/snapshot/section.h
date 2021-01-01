#ifndef TOKEN_INCLUDE_SNAPSHOT_SECTION_H
#define TOKEN_INCLUDE_SNAPSHOT_SECTION_H

#include "version.h"
#include "blockchain.h"

namespace Token{
  typedef uint64_t SnapshotSectionHeader;

  class SnapshotWriter;
  class SnapshotReader;
  class SnapshotSection{
   public:
    enum Type{
      kPrologue,
      kBlockChain,
    };

    friend std::ostream& operator<<(std::ostream& stream, const Type& type){
      switch(type){
        case Type::kPrologue:
          return stream << "Prologue";
        case Type::kBlockChain:
          return stream << "BlockChain";
        default:
          return stream << "Unknown";
      }
    }
   private:
    enum HeaderLayout{
      kTypePos = 0,
      kBitsForType = 32,

      kSizePos = kTypePos+kBitsForType,
      kBitsForSize = 32,
    };

    class TypeField : public BitField<uint64_t, SnapshotSection::Type, kTypePos, kBitsForType>{};
    class SizeField : public BitField<uint64_t, int32_t, kSizePos, kBitsForSize>{};
   protected:
    Type type_;

    SnapshotSection(const Type& type):
      type_(type){}
   public:
    virtual ~SnapshotSection() = default;

    Type GetType() const{
      return type_;
    }

    SnapshotSectionHeader GetSectionHeader() const{
      return TypeField::Encode(type_)|SizeField::Encode(GetSize());
    }

    virtual int32_t GetSize() const = 0;
    virtual bool Write(SnapshotWriter* writer) const = 0;

    static bool IsPrologueSection(const SnapshotSectionHeader& header){
      LOG(INFO) << "type: " << TypeField::Decode(header);
      return TypeField::Decode(header) == Type::kPrologue;
    }

    static bool IsBlockChainSection(const SnapshotSectionHeader& header){
      return TypeField::Decode(header) == Type::kBlockChain;
    }

    static int32_t GetSectionSize(const SnapshotSectionHeader& header){
      return SizeField::Decode(header);
    }
  };

  class SnapshotPrologueSection : public SnapshotSection{
   private:
    Timestamp timestamp_;
    Version version_;
   public:
    SnapshotPrologueSection(const Timestamp& timestamp, const Version& version):
      SnapshotSection(SnapshotSection::kPrologue),
      timestamp_(timestamp),
      version_(version){}
    SnapshotPrologueSection():
      SnapshotSection(SnapshotSection::kPrologue),
      timestamp_(GetCurrentTimestamp()),
      version_(){}
    SnapshotPrologueSection(SnapshotReader* reader);
    SnapshotPrologueSection(const SnapshotPrologueSection& section):
      SnapshotSection(SnapshotSection::kPrologue),
      timestamp_(section.timestamp_),
      version_(section.version_){}
    ~SnapshotPrologueSection() = default;

    Timestamp GetTimestamp() const{
      return timestamp_;
    }

    Version GetVersion() const{
      return version_;
    }

    int32_t GetSize() const{
      int32_t size = 0;
      size += sizeof(Timestamp);
      size += Version::kSize;
      return size;
    }

    bool Write(SnapshotWriter* writer) const;

    void operator=(const SnapshotPrologueSection& section){
      timestamp_ = section.timestamp_;
      version_ = section.version_;
    }
  };

  class SnapshotBlockChainSection : public SnapshotSection{
   private:
    Hash head_;
   public:
    SnapshotBlockChainSection(const Hash& head):
      SnapshotSection(SnapshotSection::kBlockChain),
      head_(head){}
    SnapshotBlockChainSection():
      SnapshotSection(SnapshotSection::kBlockChain),
      head_(Hash::GenerateNonce()){}
    SnapshotBlockChainSection(SnapshotReader* reader);
    SnapshotBlockChainSection(const SnapshotBlockChainSection& section):
      SnapshotSection(SnapshotSection::kBlockChain),
      head_(section.head_){}
    ~SnapshotBlockChainSection() = default;

    Hash GetHead() const{
      return head_;
    }

    int32_t GetSize() const{
      int32_t size = 0;
      size += Hash::kSize;
      return size;
    }

    bool Write(SnapshotWriter* writer) const;

    void operator=(const SnapshotBlockChainSection& section){
      head_ = section.head_;
    }
  };
}

#endif //TOKEN_INCLUDE_SNAPSHOT_SECTION_H