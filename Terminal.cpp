#include "Terminal.hpp"
#include <iostream>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

Terminal::Terminal() {
  // get current terminal options
  if (tcgetattr(STDIN_FILENO, &originalTermios)) {
    throw std::runtime_error("failed to get current terminal attributes");
  }

  struct termios raw = originalTermios;
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

void Terminal::editorDrawRows() {
  int y;
  for (y = 0; y < 24; y++) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

Terminal::~Terminal() {
  // restore old terminal config
  std::cout << "deleted\n";
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios);
}
