#include "Terminal.hpp"
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

    int c = '\0';
    // Cursor to home position again
    // TODO: maybe needed to clear screen after each keypress
    terminal.setStatusMessage("");

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
          case Terminal::ARROW_DOWN:
            terminal.moveCursor('j');
            break;
          case Terminal::ARROW_LEFT:
            terminal.moveCursor('h');
            break;
          case Terminal::ARROW_RIGHT:
            terminal.moveCursor('l');
            break;
          case Terminal::ARROW_UP:
            terminal.moveCursor('k');
            break;
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
            terminal.setStatusMessage("");
            terminal.state.terminalMode = Terminal::NORMAL;
            terminal.exitInputMode();
          } else {
            terminal.handleCharForInputMode(c);
          }
        } else if (terminal.state.terminalMode == Terminal::COMMAND) {

          if (c == 27) { // ESC returns to NORMAL mode
            inputBuffer.clear();
            terminal.state.terminalMode = Terminal::NORMAL;
          }
          if (c == '\r' || c == '\n') { // Enter processes the command
            if (inputBuffer.substr(1) == "q") {
              if (terminal.state.file_status != Terminal::MODIFIED) {
                break; // Exit if 'q' is entered
              } else {
                terminal.setStatusMessage(
                    "File has unsaved changes. To quit press :q");
              }
            } else if (inputBuffer.substr(1) == "w") {
              terminal.editorSave();
            } else if (inputBuffer.substr(1) == "wq") {
              terminal.editorSave();
              break;
            } else if (inputBuffer.substr(1) == "q!") {
              break;
            } else {
              terminal.setStatusMessage("");
            }
            terminal.state.terminalMode = Terminal::NORMAL;
            inputBuffer.clear();
          } else {

            switch (c) {
            case Terminal::SPECIAL_KEYS::CTRL_H:
            case Terminal::SPECIAL_KEYS::ESCAPE_KEY:
              inputBuffer.erase(inputBuffer.begin() + 2);
              break;
            default:
              inputBuffer += c; // Accumulate command characters
            }
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
