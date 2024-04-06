#pragma once
#include <string>
#include <termios.h>
#include <unistd.h>
class Terminal {
public:
  enum editorKey { ARROW_LEFT = 1000, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };
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
  void moveCursor(const int key);
  char editorCheckForKey(const char &key) const;

private:
  std::string buffer;
  void editorDrawRows();
  int getCursorPosition();
};
