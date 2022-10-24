#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "console_connector.h"
#include "socket_connector.h"
#include "solver.h"

class client {
  std::unique_ptr<connector> _connector;
  std::chrono::duration<double> process_timeout_s;
  bool only_logout;
  std::string remained_buffer;
  solver your_solver;

 public:
  client(std::unique_ptr<connector> conn, int process_timeout_ms, bool logout, const char token[], int level)
      : _connector(std::move(conn)), process_timeout_s(process_timeout_ms / 1000.), only_logout(logout) {
    if (!_connector->is_valid()) {
      std::cerr << "[main] "
                << "Not a valid connector" << std::endl;
      return;
    }

    std::vector<std::string> login_messages;

    if (logout) {
      login_messages.push_back(std::string("LOGOUT ") + token);
    } else {
      std::string str = "LOGIN ";
      str += token;
      if (level) {
        str += " " + std::to_string(level);
      }
      login_messages.push_back(str);
    }

    send_messages(login_messages);
  }

  void send_messages(const std::vector<std::string>& messages) {
    std::string message;

    for (std::size_t i = 0; i < messages.size(); ++i) {
      message += messages[i] + '\n';
    }
    message += ".\n";

    auto sent_bytes = _connector->send(message.c_str(), message.size());

    if (sent_bytes != static_cast<std::streamsize>(message.size())) {
      std::cerr << "[main] "
                << "Warning: Cannot sent message properly: " << message << std::endl;
      std::cerr << "[main] " << sent_bytes << " byte sent from " << message.size() << ". Closing connection."
                << std::endl;
      _connector->invalidate();
    }
  }

  std::vector<std::string> receive_message() {
    std::vector<std::string> result;

    std::string curr_buffer;
    std::swap(curr_buffer, remained_buffer);
    while (true) {
      std::string line;
      std::stringstream consumer(curr_buffer);
      while (std::getline(consumer, line)) {
        if (line == ".") {
          if (!consumer.eof()) {
            remained_buffer = consumer.str().substr(consumer.tellg());
          }
          return result;
        } else if (!line.empty()) {
          result.push_back(line);
        }
      }

      char array_buffer[512];

      auto received_bytes = _connector->recv(array_buffer, 511);

      switch (received_bytes) {
        case -1:
          std::cerr << "[main] "
                    << "Error: recv failed!" << std::endl;
          [[fallthrough]];
        case 0:
          std::cerr << "[main] "
                    << "Connection closed." << std::endl;
          _connector->invalidate();
          if (!result.empty()) {
            std::cerr << "[main] "
                      << "Latest message processing ..." << std::endl;
          }
          return result;
      }
      array_buffer[received_bytes] = '\0';
      if (!curr_buffer.empty() && curr_buffer.back() != '\n') {
        curr_buffer = result.back();
        result.pop_back();
      } else {
        curr_buffer.clear();
      }
      curr_buffer += array_buffer;
    }
  }

 public:
  void run() {
    bool firstTime = true;
    while (_connector->is_valid()) {
      auto measure_start = std::chrono::steady_clock::now();

      std::vector<std::string> tmp = receive_message();

      std::chrono::duration<double> read_seconds = std::chrono::steady_clock::now() - measure_start;
      if (read_seconds > process_timeout_s * 2) {
        std::cerr << "[main] "
                  << "Read took: " << read_seconds.count() << " seconds (>" << (process_timeout_s * 2).count() << "s)"
                  << std::endl;
      }

      if (only_logout) {
        for (auto&& s : tmp) {
          std::cerr << s << std::endl;
        }
        return;
      }

      if (tmp.empty()) {
        continue;
      }

      std::clock_t measure_clock_start = std::clock();
      measure_start = std::chrono::steady_clock::now();

      if (firstTime) {
        your_solver.startMessage(tmp);
        tmp.clear();
        firstTime = false;
      } else
        tmp = your_solver.processTick(tmp);

      std::chrono::duration<double> process_seconds = std::chrono::steady_clock::now() - measure_start;
      std::cerr << "Process took: " << process_seconds.count() << " seconds" << std::endl;
      if (process_seconds > process_timeout_s) {
        std::cerr << "[main] "
                  << "Process took: " << process_seconds.count() << " seconds (>" << process_timeout_s.count() << "s)"
                  << std::endl;
        std::cerr << "[main] "
                  << "CPU time used: " << static_cast<double>(std::clock() - measure_clock_start) / CLOCKS_PER_SEC
                  << std::endl;
      }

      if (!_connector->is_valid() || tmp.empty()) {
        continue;
      }

      send_messages(tmp);

      std::chrono::duration<double> process_with_send_seconds = std::chrono::steady_clock::now() - measure_start;
      if (process_seconds > process_timeout_s) {
        std::cerr << "[main] "
                  << "Process with send took: " << process_with_send_seconds.count() << " seconds (>"
                  << process_timeout_s.count() << "s)" << std::endl;
        std::cerr << "[main] "
                  << "CPU time used: " << static_cast<double>(std::clock() - measure_clock_start) / CLOCKS_PER_SEC
                  << " sec" << std::endl;
      }
    }
    std::cerr << "[main] "
              << "Game over" << std::endl;
  }
};

int main(int argc, char** argv) {
  if (argc > 1 && 0 == std::strcmp("help", argv[1])) {
    std::cerr << "Usage: " << std::endl
              << argv[0] << " help                   "
              << "\tPrint this message" << std::endl
              << argv[0] << " logout                 "
              << "\tSend a logout message" << std::endl
              << argv[0] << " [level] [hostname port]"
              << "\tPlay with [level] level, use tcp connection to communicate. " << std::endl
              << argv[0] << " [level] console        "
              << "\tPlay with [level] level, use console stdin and stdout to communicate" << std::endl
              << " Default level is 0 (which means random 1-10)" << std::endl;
    return 0;
  }

  const bool logout = argc > 1 && 0 == std::strcmp("logout", argv[1]);
  const int level = argc > 1 && argv[1][0] ? std::atoi(argv[1]) : 0;
  const bool from_console =
      (argc > 2 && 0 == std::strcmp("console", argv[2])) || (argc > 1 && 0 == std::strcmp("console", argv[1]));

  /* config area */
  const char* host_name = from_console ? "" : argc < 3 ? "20.82.65.82" : argv[argc - 2];
  const unsigned short port = from_console ? 0 : (argc < 3 && argv[argc - 1][0]) ? 11224 : std::atoi(argv[argc - 1]);
  const char token[] = "14XHV8dmN";

  try {
    client(from_console ? std::unique_ptr<connector>(std::make_unique<console_connector>())
                        : std::make_unique<socket_connector>(host_name, port),
           from_console ? 200 : 2000, logout, token, level)
        .run();
  } catch (std::exception& e) {
    std::cerr << "[main] "
              << "Exception throwed. what(): " << e.what() << std::endl;
  }
}
