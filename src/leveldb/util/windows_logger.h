#ifndef STORAGE_LEVELDB_UTIL_WINDOWS_LOGGER_H_
#define STORAGE_LEVELDB_UTIL_WINDOWS_LOGGER_H_

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <thread>

#include "leveldb/env.h"

namespace leveldb {

class WindowsLogger final : public Logger {
 public:

  explicit WindowsLogger(std::FILE* fp) : fp_(fp) { assert(fp != nullptr); }

  ~WindowsLogger() override { std::fclose(fp_); }

  void Logv(const char* format, va_list arguments) override {

    SYSTEMTIME now_components;
    ::GetLocalTime(&now_components);

    constexpr const int kMaxThreadIdSize = 32;
    std::ostringstream thread_stream;
    thread_stream << std::this_thread::get_id();
    std::string thread_id = thread_stream.str();
    if (thread_id.size() > kMaxThreadIdSize) {
      thread_id.resize(kMaxThreadIdSize);
    }

    constexpr const int kStackBufferSize = 512;
    char stack_buffer[kStackBufferSize];
    static_assert(sizeof(stack_buffer) == static_cast<size_t>(kStackBufferSize),
                  "sizeof(char) is expected to be 1 in C++");

    int dynamic_buffer_size = 0;
    for (int iteration = 0; iteration < 2; ++iteration) {
      const int buffer_size =
          (iteration == 0) ? kStackBufferSize : dynamic_buffer_size;
      char* const buffer =
          (iteration == 0) ? stack_buffer : new char[dynamic_buffer_size];

      int buffer_offset = snprintf(
          buffer, buffer_size, "%04d/%02d/%02d-%02d:%02d:%02d.%06d %s ",
          now_components.wYear, now_components.wMonth, now_components.wDay,
          now_components.wHour, now_components.wMinute, now_components.wSecond,
          static_cast<int>(now_components.wMilliseconds * 1000),
          thread_id.c_str());

      assert(buffer_offset <= 28 + kMaxThreadIdSize);
      static_assert(28 + kMaxThreadIdSize < kStackBufferSize,
                    "stack-allocated buffer may not fit the message header");
      assert(buffer_offset < buffer_size);

      std::va_list arguments_copy;
      va_copy(arguments_copy, arguments);
      buffer_offset +=
          std::vsnprintf(buffer + buffer_offset, buffer_size - buffer_offset,
                         format, arguments_copy);
      va_end(arguments_copy);

      if (buffer_offset >= buffer_size - 1) {

        if (iteration == 0) {

          dynamic_buffer_size = buffer_offset + 2;
          continue;
        }

        assert(false);
        buffer_offset = buffer_size - 1;
      }

      if (buffer[buffer_offset - 1] != '\n') {
        buffer[buffer_offset] = '\n';
        ++buffer_offset;
      }

      assert(buffer_offset <= buffer_size);
      std::fwrite(buffer, 1, buffer_offset, fp_);
      std::fflush(fp_);

      if (iteration != 0) {
        delete[] buffer;
      }
      break;
    }
  }

 private:
  std::FILE* const fp_;
};

}

#endif
