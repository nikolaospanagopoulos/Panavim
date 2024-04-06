#include "Terminal.hpp"
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#define EditorVersion 1

void Terminal::registerCommand(const std::string &command,
                               CommandHandler handler) {
  commandHandlers[command] = handler;
}

void Terminal::executeCommand(const std::string &command) {
  auto it = commandHandlers.find(command);
  if (it != commandHandlers.end()) {
    it->second(*this);
    state.commandBuffer.clear();
  }
}

bool Terminal::couldBeCommand(const std::string &buffer,
                              const std::vector<std::string> &commandList) {
  for (const auto &cmd : commandList) {
    if (cmd.rfind(buffer, 0) == 0) {
      return true;
    }
  }
  return false;
}

Terminal::Terminal()
    : state({.cx = 0,
             .cy = 0,
             .screenRows = 0,
             .screenCols = 0,
             .terminalMode = NORMAL,
             .commandBuffer = {}}),
      buffer{""} {
  // get current terminal options
  if (tcgetattr(STDIN_FILENO, &state.originalTermios) == -1) {
    throw std::runtime_error("failed to get current terminal attributes");
  }

  struct termios raw = state.originalTermios;
  // disable CTRL-S CTRL-Q
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  // turn of output process -> \n -> \r\n
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    throw std::runtime_error("failed to set terminal attributes");
  }
  registerCommand("gg", [](Terminal &term) { term.scrollUp(); });
  registerCommand("G", [](Terminal &term) { term.scrollDown(); });
}
void Terminal::scrollUp() {
  int times = state.screenRows;
  while (times--) {
    moveCursor('k');
  }
}
void Terminal::scrollDown() {
  int times = state.screenRows;
  while (times--) {
    moveCursor('j');
  }
}
char Terminal::editorCheckForKey(const char &key) const {
  if (key == '\x1b') {
    char seq[3]{};
    if (read(STDIN_FILENO, &seq[0], 1) != 1) {
      return '\x1b';
    }
    if (read(STDIN_FILENO, &seq[1], 1) != 1) {
      return '\x1b';
    }
    if (seq[0] == '[') {
      switch (seq[1]) {
      case 'A':
        return 'k';
      case 'B':
        return 'j';
      case 'C':
        return 'l';
      case 'D':
        return 'h';
      }
    }
    return '\x1b';
  } else {
    return key;
  }
}

void Terminal::moveCursor(const int key) {
  switch (key) {
  case editorKey::ARROW_DOWN:
  case 'j':
    if (state.cy != state.screenRows - 1) {
      state.cy++;
    }
    break;
  case editorKey::ARROW_UP:
  case 'k':
    if (state.cy != 0) {
      state.cy--;
    }
    break;
  case editorKey::ARROW_RIGHT:
  case 'l':
    if (state.cx != state.screenCols - 1) {
      state.cx++;
    }
    break;
  case editorKey::ARROW_LEFT:
  case 'h':
    if (state.cx != 0) {
      state.cx--;
    }
    break;
  }
}
void Terminal::exitInputMode() const { std::cout << "\x1b[2 q" << std::flush; }
void Terminal::editorRefreshScreen() {

  getWindowSize();
  // l command and argument 25 used to hide cursor
  buffer.append("\x1b[?25l");
  // K command -> erase each line
  // H command -> reposition cursor at top left. default argument 1
  buffer.append("\x1b[H");
  editorDrawRows();
  // H command -> reposition cursor
  std::stringstream cursorStream{};
  cursorStream << "\x1b[" << state.cy + 1 << ";" << state.cx + 1 << "H";
  buffer.append(cursorStream.str());
  // h command and argument 25 used to show cursor
  buffer.append("\x1b[?25h");
  // flush buffer on screen
  std::cout << buffer << std::flush;
  // empty it
  buffer.clear();
}

void Terminal::editorDrawRows() {
  int y;
  std::string welcomeMessage =
      "Text editor -- version " + std::to_string(EditorVersion);
  for (y = 0; y < state.screenRows; y++) {
    if (y == state.screenRows / 3) {
      // put welcomeMessage in center
      // devide screen width by two and then subtract half the string length to
      // know where to start writing add spaces for the rest
      int padding = (state.screenCols - welcomeMessage.length()) / 2;
      if (padding) {
        buffer.append("~");
        padding--;
      }
      while (padding--) {
        buffer.append(" ");
      }

      buffer.append(welcomeMessage);
    } else {
      buffer.append("~");
    }
    buffer.append("\x1b[K");
    if (y < state.screenRows - 1) {
      buffer.append("\r\n");
    }
  }
}

void Terminal::enterInputMode() const { std::cout << "\x1b[6 q" << std::flush; }
Terminal::~Terminal() {
  // restore old terminal config
  buffer.clear();
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &state.originalTermios);
  // clear screen
  std::cout << "\x1b[2J" << std::flush;
  // Move cursor to home position
  std::cout << "\x1b[H" << std::flush;
}
int Terminal::getWindowSize() {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;
    return getCursorPosition();
  } else {
    state.screenCols = ws.ws_col;
    state.screenRows = ws.ws_row;
    return 0;
  }
}
int Terminal::getCursorPosition() {
  char buf[32];
  unsigned int i = 0;
  std::cout << "\x1b[6n" << std::flush;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1)
      break;
    if (buf[i] == 'R') {
      break;
    }
    i++;
  }
  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[') {
    return -1;
  }

  std::string buffer = buf;
  std::istringstream iss(buffer.substr(2));
  char seperator;

  if (!(iss >> state.screenRows >> seperator >> state.screenCols) ||
      seperator != ';') {
    return -1;
  }

  return 0;
}
