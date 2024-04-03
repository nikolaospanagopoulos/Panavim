#pragma once
#include <termios.h>
#include <unistd.h>
class Terminal {
public:
  Terminal();
  ~Terminal();
  void editorDrawRows();

private:
  struct termios originalTermios;
};
