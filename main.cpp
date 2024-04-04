#include "Terminal.hpp"
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

int main() {
  enum MODE { NORMAL, INPUT, COMMAND }; // Added NORMAL mode for clarity

  try {
    Terminal terminal;

    std::string inputBuffer;
    MODE mode = NORMAL; // Start in NORMAL mode, similar to Vim's default mode

    char c = '\0';
    // Cursor to home position again
    // TODO: maybe needed to clear screen after each keypress

    while (true) {
      terminal.editorRefreshScreen();
      if (read(STDIN_FILENO, &c, 1) == 1) {
        if (mode == NORMAL) {
          if (c == 'i') { // 'i' to enter INPUT mode
            mode = INPUT;
            std::cout << "Entered input mode\r\n" << std::flush;

          } else if (c == ':') { // ':' to enter COMMAND mode
            mode = COMMAND;
            std::cout << "entered command mode\r\n";
            inputBuffer.clear(); // Prepare for command input
          }
          // Normal mode key handling (navigation, etc.) goes here
        } else if (mode == INPUT) {

          if (c == 27) { // ESC returns to NORMAL mode
            mode = NORMAL;
            std::cout << "Entered normal mode\r\n";
          } else {
            // Handle text input in INPUT mode
            if (std::iscntrl(c)) {
              std::cout << static_cast<int>(c) << "\r\n";
            } else {
              std::cout << static_cast<int>(c) << " ('" << c << "')\r\n";
            }
          }
        } else if (mode == COMMAND) {

          if (c == 27) { // ESC returns to NORMAL mode
            mode = NORMAL;
            std::cout << "Entered normal mode\r\n";
          }
          if (c == '\r' || c == '\n') { // Enter processes the command
            if (inputBuffer == "q") {
              break; // Exit if 'q' is entered
            }
            mode = NORMAL; // Return to NORMAL mode after command execution
            inputBuffer.clear();
          } else {
            inputBuffer += c; // Accumulate command characters
          }
        }
      }
    }
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return 0;
}
