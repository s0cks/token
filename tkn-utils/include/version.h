#ifndef TOKEN_VERSION_H
#define TOKEN_VERSION_H

#include <cstring>
#include <sstream>

namespace token{
#define TOKEN_VERSION_MAJOR 0
#define TOKEN_VERSION_MINOR 0
#define TOKEN_VERSION_REVISION 1

  class Version{
   private:
    static inline int
    Compare(const Version& a, const Version& b){
      if(a.major() < b.major()){
        return -1;
      } else if(a.major() > b.major()){
        return +1;
      }

      if(a.minor() < b.minor()){
        return -1;
      } else if(a.minor() > b.minor()){
        return +1;
      }

      if(a.revision() < b.revision()){
        return -1;
      } else if(a.revision() > b.revision()){
        return +1;
      }
      return 0;
    }
   private:
    int16_t major_;
    int16_t minor_;
    int16_t revision_;

   public:
    Version() = default;
    Version(const int16_t& major, const int16_t& minor, const int16_t& revision):
      major_(major),
      minor_(minor),
      revision_(revision){}
     Version(const Version& version) = default;
    ~Version() = default;

    int16_t major() const{
      return major_;
    }

    int16_t minor() const{
      return minor_;
    }

    int16_t revision() const{
      return revision_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "Version(";
      ss << "major=" << major() << ", ";
      ss << "minor=" << minor() << ", ";
      ss << "revision=" << revision();
      ss << ")";
      return ss.str();
    }

    Version& operator=(const Version& other) = default;

    friend bool operator==(const Version& a, const Version& b){
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const Version& a, const Version& b){
      return Compare(a, b) != 0;
    }

    friend bool operator<=(const Version& a, const Version& b){
      return Compare(a, b) <= 0;
    }

    friend bool operator<(const Version& a, const Version& b){
      return Compare(a, b) < 0;
    }

    friend bool operator>=(const Version& a, const Version& b){
      return Compare(a, b) >= 0;
    }

    friend bool operator>(const Version& a, const Version& b){
      return Compare(a, b) > 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Version& version){
      return stream << version.ToString();
    }

    static inline Version
    CurrentVersion(){
      return Version(TOKEN_VERSION_MAJOR, TOKEN_VERSION_MINOR, TOKEN_VERSION_REVISION);
    }
  };
}

#endif //TOKEN_VERSION_H