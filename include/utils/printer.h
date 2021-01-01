#ifndef TOKEN_PRINTER_H
#define TOKEN_PRINTER_H

#include "block.h"

namespace Token{
  class Printer{
   public:
    static const int kFlagNone = 0;
    static const int kFlagDetailed = 1 << 1;
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

    bool Print(const BlockPtr& blk) const{
      LOG_AT_LEVEL(GetSeverity()) << "Block #" << blk->GetHeight() << ":";
      LOG_AT_LEVEL(GetSeverity()) << "Hash: " << blk->GetHash();
      LOG_AT_LEVEL(GetSeverity()) << "Created: " << GetTimestampFormattedReadable(blk->GetTimestamp());
      LOG_AT_LEVEL(GetSeverity()) << "Previous: " << blk->GetPreviousHash();
      LOG_AT_LEVEL(GetSeverity()) << "Merkle Root: " << blk->GetMerkleRoot();
      LOG_AT_LEVEL(GetSeverity()) << "Number of Transactions: " << blk->GetNumberOfTransactions();
      return true;
    }

    bool Print(const TransactionPtr& tx) const{
      LOG_AT_LEVEL(GetSeverity()) << "Transaction #" << tx->GetIndex() << ":";
      LOG_AT_LEVEL(GetSeverity()) << "Hash: " << tx->GetHash();
      LOG_AT_LEVEL(GetSeverity()) << "Created: " << GetTimestampFormattedReadable(tx->GetTimestamp());
      LOG_AT_LEVEL(GetSeverity()) << "Number of Inputs: " << tx->GetNumberOfInputs();
      LOG_AT_LEVEL(GetSeverity()) << "Number of Outputs: " << tx->GetNumberOfOutputs();
      return true;
    }

    bool Print(const UnclaimedTransactionPtr& utxo) const{
      LOG_AT_LEVEL(GetSeverity()) << "Unclaimed Transaction " << utxo->GetHash();
      LOG_AT_LEVEL(GetSeverity()) << "Location: " << utxo->GetTransaction() << "[" << utxo->GetIndex() << "]";
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
  };
}

#endif //TOKEN_PRINTER_H