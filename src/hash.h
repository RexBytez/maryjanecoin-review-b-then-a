#ifndef BITCOIN_HASH_H
#define BITCOIN_HASH_H

#include "uint256.h"
#include "serialize.h"

#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <vector>

unsigned int MurmurHash3(unsigned int nHashSeed, const std::vector<unsigned char>& vDataToHash);

#endif
