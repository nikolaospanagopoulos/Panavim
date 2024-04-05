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

Terminal::Terminal()
    : state({.cx = 0, .cy = 0, .screenRows = 0, .screenCols = 0}), buffer{""} {
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
}
void Terminal::moveCursor(char key) {
  switch (key) {
  case 'j':
	  state.cy++;
    break;
  case 'k':
	  state.cy--;
    break;
  case 'l':
	  state.cx++;
    break;
  case 'h':
	  state.cx--;
    break;
  }
}
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

Terminal::~Terminal() {
  // restore old terminal config
  buffer.clear();
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &state.originalTermios);
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
