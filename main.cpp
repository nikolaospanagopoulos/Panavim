#include "Terminal.hpp"
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

int main(int argc, char *argv[]) {

  try {
    Terminal terminal;
    if (argc >= 2) {
      terminal.editorOpen(argv[1]);
    }

    std::string normalModeBuffer{};
    std::string inputBuffer;

    char c = '\0';
    // Cursor to home position again
    // TODO: maybe needed to clear screen after each keypress
    terminal.setStatusMessage("ddddd");

    while (true) {
      terminal.editorRefreshScreen();

      if (read(STDIN_FILENO, &c, 1) == 1) {
        c = terminal.editorCheckForKey(c);
        if (terminal.state.terminalMode == Terminal::NORMAL) {
          terminal.setStatusMessage("");
          terminal.state.commandBuffer += c;
          if (c == 'i') { // 'i' to enter INPUT mode

            terminal.setStatusMessage("--INPUT--");
            terminal.state.terminalMode = Terminal::INPUT;
            terminal.enterInputMode();
            inputBuffer.clear(); // Prepare for command input

          } else if (c == ':') { // ':' to enter COMMAND mode
            terminal.state.terminalMode = Terminal::COMMAND;
            inputBuffer.clear(); // Prepare for command input
            inputBuffer = ":";
            terminal.setStatusMessage(inputBuffer);
          }
          // Normal mode key handling (navigation, etc.) goes here
          switch (c) {
          case 'k':
          case 'j':
          case 'l':
          case 'h':
            terminal.moveCursor(c);
            break;
          }
          if (!terminal.couldBeCommand(terminal.state.commandBuffer,
                                       std::vector<std::string>{"}", "{", "gg",
                                                                "G", "_", "$",
                                                                "w", "b"})) {
            terminal.state.commandBuffer.clear();
          }
          terminal.executeCommand(terminal.state.commandBuffer);

          if (c == 27) {
            terminal.state.commandBuffer.clear();
          }
        } else if (terminal.state.terminalMode == Terminal::INPUT) {

          if (c == 27) { // ESC returns to NORMAL mode
            terminal.state.terminalMode = Terminal::NORMAL;
            terminal.exitInputMode();
          } else {
            // Handle text input in INPUT mode
          }
        } else if (terminal.state.terminalMode == Terminal::COMMAND) {

          if (c == 27) { // ESC returns to NORMAL mode
            inputBuffer.clear();
            terminal.state.terminalMode = Terminal::NORMAL;
          }
          if (c == '\r' || c == '\n') { // Enter processes the command
            if (inputBuffer.substr(1) == "q") {
              break; // Exit if 'q' is entered
            }
            terminal.state.terminalMode = Terminal::NORMAL;
            terminal.setStatusMessage("");
            inputBuffer.clear();
          } else {
            inputBuffer += c; // Accumulate command characters

            terminal.setStatusMessage(inputBuffer);
          }
        }
      }
    }
  } catch (const std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return 0;
}
