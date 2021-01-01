#ifndef TOKEN_INCLUDE_MERKLE_PROOF_H
#define TOKEN_INCLUDE_MERKLE_PROOF_H

namespace Token{
  class MerkleProofHash{
   public:
    enum Direction{
      kRoot,
      kLeft,
      kRight,
    };
   private:
    Direction direction_;
    Hash hash_;
   public:
    MerkleProofHash(Direction direction, const Hash& hash):
      direction_(direction),
      hash_(hash){}
    MerkleProofHash(Direction direction, const MerkleNode& node):
      direction_(direction),
      hash_(node.GetHash()){}
    MerkleProofHash(const MerkleProofHash& other):
      direction_(other.direction_),
      hash_(other.hash_){}
    ~MerkleProofHash(){}

    Direction GetDirection() const{
      return direction_;
    }

    bool IsRoot() const{
      return GetDirection() == Direction::kRoot;
    }

    bool IsLeft() const{
      return GetDirection() == Direction::kLeft;
    }

    bool IsRight() const{
      return GetDirection() == Direction::kRight;
    }

    Hash GetHash() const{
      return hash_;
    }

    void operator=(const MerkleProofHash& other){
      direction_ = other.direction_;
      hash_ = other.hash_;
    }

    friend bool operator==(const MerkleProofHash& a, const MerkleProofHash& b){
      return a.hash_ == b.hash_
        && a.direction_ == b.direction_;
    }

    friend bool operator!=(const MerkleProofHash& a, const MerkleProofHash& b){
      return a.hash_ != b.hash_
        && a.direction_ != b.direction_;
    }
  };

  typedef std::vector<MerkleProofHash> MerkleProof;
}

#endif //TOKEN_INCLUDE_MERKLE_PROOF_H
