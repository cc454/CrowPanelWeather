#pragma once
#include <cstdint>
#include <map>
#include <string>
using String = std::string;
#define FILE_READ 0
#define FILE_WRITE 1
namespace storage_test {
inline bool inserted = false, allowSave = true;
inline unsigned mounts = 0;
inline std::map<std::string, std::string> preferences;
} // namespace storage_test
class File {
  std::string *contents = nullptr;
  size_t position = 0;

public:
  File() = default;
  explicit File(std::string *data) : contents(data) {}
  explicit operator bool() const { return contents != nullptr; }
  size_t size() const { return contents ? contents->size() : 0; }
  String readString() {
    auto result = contents->substr(position);
    position = contents->size();
    return result;
  }
  String readStringUntil(char delimiter) {
    auto end = contents->find(delimiter, position);
    auto result = contents->substr(position, end - position);
    position = end == std::string::npos ? contents->size() : end + 1;
    return result;
  }
  size_t write(const uint8_t *data, size_t count) {
    contents->append(reinterpret_cast<const char *>(data), count);
    return count;
  }
  void flush() {}
  void close() {}
};
namespace fs {
class FS {
public:
  bool mounted = false;
  std::map<std::string, std::string> files;
  bool exists(const String &name) const { return mounted && files.count(name); }
  File open(const String &name, int mode = FILE_READ) {
    if (!mounted || (mode == FILE_READ && !files.count(name)))
      return File();
    return File(&files[name]);
  }
  bool remove(const String &name) { return mounted && files.erase(name); }
  bool rename(const String &a, const String &b) {
    if (!exists(a))
      return false;
    files[b] = files[a];
    files.erase(a);
    return true;
  }
};
} // namespace fs
