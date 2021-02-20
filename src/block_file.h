#ifndef TOKEN_BLOCK_WRITER_H
#define TOKEN_BLOCK_WRITER_H

#include <cstdio>
#include "utils/filesystem.h"

/**
 * Block FILE:
 * --------------------------------------------------
 *     Tag(Block, xxxx)                   8 Bytes
 * --------------------------------------------------
 *     Header                           128 Bytes
 * --------------------------------------------------
 *     Transaction Data
 *     (40)+(|T|*((104*|tI|)+(128*|tO|))) Bytes
 */
namespace token{
  bool WriteBlock(FILE* file, const BlockPtr& blk);
  BlockPtr ReadBlock(FILE* file);
}

#endif//TOKEN_BLOCK_WRITER_H