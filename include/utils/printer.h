#ifndef TOKEN_PRINTER_H
#define TOKEN_PRINTER_H

#include "block.h"
#include "block_header.h"

namespace token{
  class Printer{
   public:
    static const int kFlagNone;
    static const int kFlagDetailed;
   protected:
    Printer* parent_;
    google::LogSeverity severity_;
    long flags_;

    Printer(Printer* parent):
      parent_(parent),
      severity_(parent->GetSeverity()),
      flags_(parent->flags_){}
    Printer(const google::LogSeverity& severity, const long& flags):
      parent_(nullptr),
      severity_(severity),
      flags_(flags){}

    long GetFlags() const{
      return flags_;
    }
   public:
    virtual ~Printer() = default;

    Printer* GetParent() const{
      return parent_;
    }

    bool IsDetailed() const{
      return (flags_ & kFlagDetailed) == kFlagDetailed;
    }

    google::LogSeverity GetSeverity() const{
      return severity_;
    }
  };

  class ToStringPrinter : public Printer{
   public:
    ToStringPrinter(const google::LogSeverity severity = google::INFO, const long& flags = Printer::kFlagNone):
      Printer(severity, flags){}
    ToStringPrinter(Printer* parent):
      Printer(parent){}
    ~ToStringPrinter() = default;

    bool Print(Object* obj) const{
      LOG_AT_LEVEL(GetSeverity()) << obj->ToString();
      return true;
    }
  };

  class HashPrinter : public Printer{
   public:
    HashPrinter(const google::LogSeverity& severity = google::INFO, const long& flags = Printer::kFlagNone):
      Printer(severity, flags){}
    HashPrinter(Printer* parent):
      Printer(parent){}
    ~HashPrinter() = default;

    bool Print(BinaryObject* obj) const{
      LOG_AT_LEVEL(GetSeverity()) << obj->GetHash();
      return true;
    }
  };

  class PrettyPrinter : Printer{
   public:
    PrettyPrinter(const google::LogSeverity& severity = google::INFO, const long& flags = Printer::kFlagNone):
      Printer(severity, flags){}
    ~PrettyPrinter() = default;

    bool Print(const BlockHeader& blk) const{
      if(IsDetailed()){
        LOG_AT_LEVEL(GetSeverity()) << "Block #" << blk.GetHeight() << ":";
        LOG_AT_LEVEL(GetSeverity()) << "Hash: " << blk.GetHash();
        LOG_AT_LEVEL(GetSeverity()) << "Created: " << FormatTimestampReadable(blk.GetTimestamp());
        LOG_AT_LEVEL(GetSeverity()) << "Previous: " << blk.GetPreviousHash();
        LOG_AT_LEVEL(GetSeverity()) << "Merkle Root: " << blk.GetMerkleRoot();
        LOG_AT_LEVEL(GetSeverity()) << "Number of Transactions: " << blk.GetNumberOfTransactions();
      } else{
        LOG_AT_LEVEL(GetSeverity()) << "#" << blk.GetHeight() << "(" << blk.GetHash() << ")";
      }
      return true;
    }

    bool Print(const BlockPtr& blk) const{
      LOG_AT_LEVEL(GetSeverity()) << "Block #" << blk->GetHeight() << ":";
      LOG_AT_LEVEL(GetSeverity()) << "Hash: " << blk->GetHash();
      LOG_AT_LEVEL(GetSeverity()) << "Created: " << FormatTimestampReadable(blk->GetTimestamp());
      LOG_AT_LEVEL(GetSeverity()) << "Previous: " << blk->GetPreviousHash();
      LOG_AT_LEVEL(GetSeverity()) << "Merkle Root: " << blk->GetMerkleRoot();
      if(!IsDetailed()){
        LOG_AT_LEVEL(GetSeverity()) << "Number of Transactions: " << blk->GetNumberOfTransactions();
        return true;
      }

      LOG_AT_LEVEL(GetSeverity()) << "Transactions:";
      for(auto& tx : blk->transactions())
        Print(tx);
      return true;
    }

    bool Print(const TransactionPtr& tx) const{
      LOG_AT_LEVEL(GetSeverity()) << "Transaction #" << tx->GetIndex() << ":";
      LOG_AT_LEVEL(GetSeverity()) << "Hash: " << tx->GetHash();
      LOG_AT_LEVEL(GetSeverity()) << "Created: " << FormatTimestampReadable(tx->GetTimestamp());
      if(!IsDetailed()){
        LOG_AT_LEVEL(GetSeverity()) << "Number of Inputs: " << tx->inputs_size();
      } else{
        LOG_AT_LEVEL(GetSeverity()) << "Inputs (" << tx->inputs_size() << "):";
        for(auto& it : tx->inputs()){
          LOG_AT_LEVEL(GetSeverity()) << " - " << it;
        }
      }

      if(!IsDetailed()){
        LOG_AT_LEVEL(GetSeverity()) << "Number of Outputs: " << tx->outputs_size();
      } else{
        LOG_AT_LEVEL(GetSeverity()) << "Outputs (" << tx->outputs_size() << "):";
        for(auto& it : tx->outputs()){
          LOG_AT_LEVEL(GetSeverity()) << " - " << it;
        }
      }
      return true;
    }

    bool Print(const UnclaimedTransactionPtr& utxo) const{
      LOG_AT_LEVEL(GetSeverity()) << "Unclaimed Transaction " << utxo->GetHash();
      LOG_AT_LEVEL(GetSeverity()) << "Location: " << utxo->GetReference();
      LOG_AT_LEVEL(GetSeverity()) << "User: " << utxo->GetUser();
      LOG_AT_LEVEL(GetSeverity()) << "Product: " << utxo->GetProduct();
      return true;
    }

    bool operator()(const BlockPtr& blk) const{
      return Print(blk);
    }

    bool operator()(const TransactionPtr& tx) const{
      return Print(tx);
    }

    bool operator()(const UnclaimedTransactionPtr& utxo) const{
      return Print(utxo);
    }

    static inline bool
    PrettyPrint(const BlockPtr& blk, const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone){
      PrettyPrinter printer(severity, flags);
      return printer.Print(blk);
    }

    static inline bool
    PrettyPrint(const TransactionPtr& tx, const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone){
      PrettyPrinter printer(severity, flags);
      return printer.Print(tx);
    }

    static inline bool
    PrettyPrint(const UnclaimedTransactionPtr& utxo, const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone){
      PrettyPrinter printer(severity, flags);
      return printer.Print(utxo);
    }
  };

  class BannerPrinter : Printer{
   public:
    static const int kDefaultSize = 75;
   private:
    int size_;

    static inline std::string
    CreateBanner(){
      std::stringstream ss;
      ss << "token v" << token::GetVersion();
#ifdef TOKEN_DEBUG
      ss << " (Debug Mode Enabled)";
#endif//TOKEN_DEBUG
      return ss.str();
    }

    static inline std::string
    CreateMarquee(int size){
      std::string text = CreateBanner();

      int offset = (size - text.size()) / 2;

      std::stringstream ss;
      ss << '#';
      int i;
      for(i = 0; i < offset - 2; i++)
        ss << ' ';
      ss << text;
      for(i = (i + text.size()); i < size - 2; i++)
        ss << ' ';
      ss << '#';
      return ss.str();
    }

    static inline std::string
    CreateGutter(int size){
      std::stringstream ss;
      for(int i = 0; i < size; i++)
        ss << '#';
      return ss.str();
    }
   public:
    BannerPrinter(int size = BannerPrinter::kDefaultSize,
      const google::LogSeverity& severity = google::INFO,
      const long& flags = Printer::kFlagNone):
      Printer(severity, flags),
      size_(size){}
    BannerPrinter(Printer* parent, int size = BannerPrinter::kDefaultSize):
      Printer(parent),
      size_(size){}
    ~BannerPrinter() = default;

    int GetSize() const{
      return size_;
    }

    bool Print() const{
      LOG_AT_LEVEL(GetSeverity()) << CreateGutter(GetSize());
      LOG_AT_LEVEL(GetSeverity()) << CreateMarquee(GetSize());
      LOG_AT_LEVEL(GetSeverity()) << CreateGutter(GetSize());
      return true;
    }

    static inline bool
    PrintBanner(Printer* parent, int size = BannerPrinter::kDefaultSize){
      BannerPrinter printer(parent, size);
      return printer.Print();
    }

    static inline bool
    PrintBanner(int size = BannerPrinter::kDefaultSize,
      const google::LogSeverity& severity = google::INFO,
      const long& flags = Printer::kFlagNone){
      BannerPrinter printer(size, severity, flags);
      return printer.Print();
    }
  };
}

#endif //TOKEN_PRINTER_H