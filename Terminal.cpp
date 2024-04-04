#include "Terminal.hpp"
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

Terminal::Terminal() : state({.screenRows = 0, .screenCols = 0}) {
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
void Terminal::editorRefreshScreen() {

  getWindowSize();
  // refactor to std::flush
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  editorDrawRows();
  write(STDOUT_FILENO, "\x1b[H", 3);
}

void Terminal::editorDrawRows() {
  int y;
  for (y = 0; y < state.screenRows; y++) {
    write(STDOUT_FILENO, "~", 1);
    if (y < state.screenRows - 1) {
      write(STDOUT_FILENO, "\r\n", 2);
    }
  }
}

Terminal::~Terminal() {
  // restore old terminal config
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
