#pragma once
#include <cstring>
#include <vector>
#include <type_traits>
#include <cstdint>

class Bitstream
{
  std::vector<char> buffer;
  size_t readPos = 0;

public:
  Bitstream() = default;

  Bitstream(uint8_t* data, size_t size)
    : buffer(data, data + size), readPos(0) {}

  const char *data() const { return buffer.data(); }
  size_t size() const { return buffer.size(); }

  template<typename T>
  void write(const T &val)
  {
    static_assert(std::is_trivially_copyable<T>::value, "Only trivially copyable types allowed");
    const char *ptr = reinterpret_cast<const char *>(&val);
    buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
  }

  template<typename T>
  void read(T &val)
  {
    static_assert(std::is_trivially_copyable<T>::value, "Only trivially copyable types allowed");
    std::memcpy(&val, buffer.data() + readPos, sizeof(T));
    readPos += sizeof(T);
  }
};
