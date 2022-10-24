#ifndef CONSOLE_CONNECTOR_H_INCLUDED
#define CONSOLE_CONNECTOR_H_INCLUDED

#include <cstring>
#include <iostream>

#include "connector.h"

class console_connector final : public connector {
 public:
  console_connector() { std::ios::sync_with_stdio(false); }

  virtual ~console_connector(){};

  virtual std::streamsize send(const char* data, std::streamsize size) override {
    std::cout.write(data, size);
    std::cout.flush();
    return std::cout ? size : 0;
  }

  virtual std::streamsize recv(char* buffer, std::streamsize size) override {
    std::memset(buffer, 0, size);
    auto res = std::cin.getline(buffer, size).gcount();
    if (std::cin.fail() && !std::cin.eof()) {
      res = size - 1;
      std::cin.clear();
    }
    if (res > 0 && buffer[res - 1] == '\0') {
      buffer[res - 1] = '\n';
    }
    return res;
  }
};

#endif  // CONSOLE_CONNECTOR_H_INCLUDED
