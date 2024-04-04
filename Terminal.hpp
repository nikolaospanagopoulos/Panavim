#pragma once
#include <termios.h>
#include <unistd.h>
class Terminal {
public:
  struct editorState {
    int screenRows;
    int screenCols;
    struct termios originalTermios;
  } state;
  Terminal();
  ~Terminal();
  void editorRefreshScreen();
  int getWindowSize();

private:
  void editorDrawRows();
  int getCursorPosition();
};
