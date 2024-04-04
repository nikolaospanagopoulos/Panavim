#include "Terminal.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

Terminal::Terminal()
    : state({.screenRows = 0, .screenCols = 0, .originalTermios = {}}) {
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
  getWindowSize();
}
void Terminal::editorRefreshScreen() const {
  // refactor to std::flush
  std::cout << "\x1b[2J" << std::flush;
  std::cout << "\x1b[H" << std::flush;
  editorDrawRows();
  std::cout << "\x1b[H" << std::flush;
}

void Terminal::editorDrawRows() const {
  for (int y = 0; y < state.screenRows; y++) {

    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

Terminal::~Terminal() {
  // restore old terminal config
  std::cout << "deleted\n";
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &state.originalTermios);
}
int Terminal::getWindowSize() {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  } else {
    state.screenCols = ws.ws_col;
    state.screenRows = ws.ws_row;
    return 0;
  }
}
