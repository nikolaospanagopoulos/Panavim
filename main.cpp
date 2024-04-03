#include "Terminal.hpp"
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

int main() {

  try {
    Terminal terminal;
    std::string inputBuffer;

    bool isCommandMode{false};
    char c = '\0';
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    terminal.editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);

    while (true) {
      if (read(STDIN_FILENO, &c, 1) == 1) {
        if (!isCommandMode) {
          if (std::iscntrl(c)) {
            std::cout << static_cast<int>(c) << "\r\n";
          } else {
            std::cout << static_cast<int>(c) << " ('" << c << "')\r\n";
          }
        }
        if (c == '\r' || c == '\n') {
          if (isCommandMode) {
            if (inputBuffer == "q") {
              write(STDOUT_FILENO, "\x1b[2J", 4);
              write(STDOUT_FILENO, "\x1b[H", 3);
              break;
            }
            inputBuffer.clear();
            isCommandMode = false;
          } else {
            // normal input
          }
        } else if (c == ':') {
          isCommandMode = true;
          inputBuffer.clear();
        } else if (isCommandMode) {
          inputBuffer += c;
        } else {
        }
      }
    }

  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return 0;
}
