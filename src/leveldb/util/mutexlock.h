#ifndef STORAGE_LEVELDB_UTIL_MUTEXLOCK_H_
#define STORAGE_LEVELDB_UTIL_MUTEXLOCK_H_

#include "port/port.h"
#include "port/thread_annotations.h"

namespace leveldb {

class SCOPED_LOCKABLE MutexLock {
 public:
  explicit MutexLock(port::Mutex* mu) EXCLUSIVE_LOCK_FUNCTION(mu) : mu_(mu) {
    this->mu_->Lock();
  }
  ~MutexLock() UNLOCK_FUNCTION() { this->mu_->Unlock(); }

  MutexLock(const MutexLock&) = delete;
  MutexLock& operator=(const MutexLock&) = delete;

 private:
  port::Mutex* const mu_;
};

}

#endif
