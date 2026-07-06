#ifndef STORAGE_LEVELDB_UTIL_ENV_WINDOWS_TEST_HELPER_H_
#define STORAGE_LEVELDB_UTIL_ENV_WINDOWS_TEST_HELPER_H_

namespace leveldb {

class EnvWindowsTest;

class EnvWindowsTestHelper {
 private:
  friend class CorruptionTest;
  friend class EnvWindowsTest;

  static void SetReadOnlyMMapLimit(int limit);
};

}

#endif
