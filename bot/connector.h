#ifndef CONNECTOR_H_INCLUDED
#define CONNECTOR_H_INCLUDED

#include <ios>

class connector {
  bool valid = true;

 public:
  virtual std::streamsize send(const char*, std::streamsize) = 0;
  virtual std::streamsize recv(char*, std::streamsize) = 0;

  virtual void invalidate() { valid = false; }

  virtual bool is_valid() const { return valid; }

  virtual ~connector(){};
};

#endif  // CONNECTOR_H_INCLUDED
