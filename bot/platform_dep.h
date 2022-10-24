#ifndef PLATFORM_DEP_H_INCLUDED
#define PLATFORM_DEP_H_INCLUDED

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__TOS_WIN__) || defined(__WINDOWS__)
#ifndef _WIN32_WINNT         // Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0501  // Change this to the appropriate value to target other versions of Windows.
#endif
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#undef min
#undef max
#include <stdexcept>
#define socketerrno WSAGetLastError()
#pragma comment(lib, "ws2_32.lib")
#else  // not windows
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#define socketerrno errno
#endif

namespace platform_dep {
class enable_socket {
#ifdef WIN32
 public:
  enable_socket() {
    WSADATA WSAData;

    if (WSAStartup(0x101, &WSAData) != 0) {
      throw std::runtime_error("Error: Cannot able to use WINSOCK");
    }
  }
  ~enable_socket() { WSACleanup(); }
#endif  // WIN32
};

class tcp_socket {
  SOCKET handler;

 public:
  tcp_socket() : handler(socket(AF_INET, SOCK_STREAM, 0)) {}

  ~tcp_socket() { invalidate(); }

  bool valid() const { return handler != INVALID_SOCKET; }

  SOCKET get_handler() const { return handler; }

  void invalidate() {
    if (valid()) {
      closesocket(handler);
      handler = INVALID_SOCKET;
    }
  }
};
}  // namespace platform_dep

#endif  // PLATFORM_DEP_H_INCLUDED
