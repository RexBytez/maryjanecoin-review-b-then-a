#ifndef STORAGE_LEVELDB_PORT_PORT_EXAMPLE_H_
#define STORAGE_LEVELDB_PORT_PORT_EXAMPLE_H_

#include "port/thread_annotations.h"

namespace leveldb {
namespace port {

static const bool kLittleEndian = true ;

class LOCKABLE Mutex {
 public:
  Mutex();
  ~Mutex();

  void Lock() EXCLUSIVE_LOCK_FUNCTION();

  void Unlock() UNLOCK_FUNCTION();

  void AssertHeld() ASSERT_EXCLUSIVE_LOCK();
};

class CondVar {
 public:
  explicit CondVar(Mutex* mu);
  ~CondVar();

  void Wait();

  void Signal();

  void SignallAll();
};

bool Snappy_Compress(const char* input, size_t input_length,
                     std::string* output);

bool Snappy_GetUncompressedLength(const char* input, size_t length,
                                  size_t* result);

bool Snappy_Uncompress(const char* input_data, size_t input_length,
                       char* output);

bool GetHeapProfile(void (*func)(void*, const char*, int), void* arg);

uint32_t AcceleratedCRC32C(uint32_t crc, const char* buf, size_t size);

}
}

#endif
