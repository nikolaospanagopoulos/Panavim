#pragma once
#include <string>
#include <termios.h>
#include <unistd.h>
class Terminal {
public:
  struct editorState {
    int cx;
    int cy;
    int screenRows;
    int screenCols;
    struct termios originalTermios;
  } state;
  Terminal();
  ~Terminal();
  void editorRefreshScreen();
  int getWindowSize();
  void moveCursor(char key);

private:
  std::string buffer;
  void editorDrawRows();
  int getCursorPosition();
};
