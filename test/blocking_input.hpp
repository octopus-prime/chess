#pragma once

#include <fstream>
#include <span>
#include <string_view>
#include <mutex>

class blocking_input {
  std::ifstream stream;
  std::mutex mutex;

public:
  blocking_input(std::string_view file) : stream{file.data()}, mutex{} {}

  bool read(std::span<char, 256> epd) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!stream.good())
      return false;
    stream.getline(epd.data(), epd.size());
    return true;
  }
};
