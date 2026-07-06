#ifndef STORAGE_LEVELDB_UTIL_LOGGING_H_
#define STORAGE_LEVELDB_UTIL_LOGGING_H_

#include <stdint.h>
#include <stdio.h>

#include <string>

#include "port/port.h"

namespace leveldb {

class Slice;
class WritableFile;

void AppendNumberTo(std::string* str, uint64_t num);

void AppendEscapedStringTo(std::string* str, const Slice& value);

std::string NumberToString(uint64_t num);

std::string EscapeString(const Slice& value);

bool ConsumeDecimalNumber(Slice* in, uint64_t* val);

}

#endif
