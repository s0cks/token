#ifndef TOKEN_VERSION_H
#define TOKEN_VERSION_H

#include "utils/bitfield.h"

namespace token{
  typedef uint64_t RawVersion;

  class Version{
   public:
    static inline int
    Compare(const Version& a, const Version& b){
      if(!a.IsValid())
        LOG(WARNING) << "a is not IsValid. (GetMagic=" << std::hex << MagicField::Decode(a.raw()) << ")";

      if(!b.IsValid())
        LOG(WARNING) << "b is not IsValid. (GetMagic=" << std::hex << MagicField::Decode(b.raw()) << ")";

      if(a.GetMajor() < b.GetMajor()){
        return -1;
      } else if(a.GetMajor() > b.GetMajor()){
        return +1;
      }

      if(a.GetMinor() < b.GetMinor()){
        return -1;
      } else if(a.GetMinor() > b.GetMinor()){
        return +1;
      }

      if(a.GetRevision() < b.GetRevision()){
        return -1;
      } else if(a.GetRevision() > b.GetRevision()){
        return +1;
      }
      return 0;
    }
   private:
    enum Layout{
      kMagicPosition = 0,
      kBitsForMagic = 16,

      kMajorPosition = kMagicPosition+kBitsForMagic,
      kBitsForMajor = 16,

      kMinorPosition = kMajorPosition+kBitsForMajor,
      kBitsForMinor = 16,

      kRevisionPosition = kMinorPosition+kBitsForMinor,
      kBitsForRevision = 16,

      kTotalSize = kRevisionPosition+kBitsForRevision,
    };

    RawVersion raw_;

    class MagicField : public BitField<RawVersion, uint16_t, kMagicPosition, kBitsForMagic>{};
    class MajorField : public BitField<RawVersion, uint16_t, kMajorPosition, kBitsForMajor>{};
    class MinorField : public BitField<RawVersion, uint16_t, kMinorPosition, kBitsForMinor>{};
    class RevisionField : public BitField<RawVersion, uint16_t, kRevisionPosition, kBitsForRevision>{};
   public:
    Version(const RawVersion& raw):
      raw_(raw){}
    Version(const uint16_t major, const uint16_t minor, const uint16_t revision):
      raw_(MagicField::Encode(TOKEN_MAGIC)
          |MajorField::Encode(major)
          |MinorField::Encode(minor)
          |RevisionField::Encode(revision)){}
    Version(const Version& version):
      raw_(version.raw_){}
    ~Version() = default;

    RawVersion raw() const{
      return raw_;
    }

    uint16_t GetMagic() const{
      return MagicField::Decode(raw_);
    }

    uint16_t GetMajor() const{
      return MajorField::Decode(raw_);
    }

    uint16_t GetMinor() const{
      return MinorField::Decode(raw_);
    }

    uint16_t GetRevision() const{
      return RevisionField::Decode(raw_);
    }

    bool IsValid() const{
      return GetMagic() == TOKEN_MAGIC;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << GetMajor() << '.' << GetMinor() << '.' << GetRevision();
      return ss.str();
    }

    Version& operator=(const Version& version){
      raw_ = version.raw_;
      return (*this);
    }

    friend bool operator==(const Version& a, const Version& b){
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const Version& a, const Version& b){
      return Compare(a, b) != 0;
    }

    friend bool operator<(const Version& a, const Version& b){
      return Compare(a, b) < 0;
    }

    friend bool operator>(const Version& a, const Version& b){
      return Compare(a, b) > 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Version& version){
      return stream << version.ToString();
    }
  };
}

#endif //TOKEN_VERSION_H