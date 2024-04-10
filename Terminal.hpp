#pragma once
#include <fstream>
#include <functional>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <unordered_map>
class Terminal {
public:
  enum editorKey { ARROW_LEFT = 1000, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };
  enum MODE { NORMAL, INPUT, COMMAND }; // Added NORMAL mode for clarity
  using CommandHandler = std::function<void(Terminal &)>;
  struct editorState {
    int cx;
    int cy;
    int screenRows;
    int screenCols;
    struct termios originalTermios;
    MODE terminalMode;
    std::string commandBuffer;
    int numRow;
    std::vector<std::string> textRows;
    int rowOffset;
    int colOffset;
  } state;
  Terminal();
  ~Terminal();
  void editorRefreshScreen();
  int getWindowSize();
  void moveCursor(const int key);
  char editorCheckForKey(const char &key) const;
  void enterInputMode() const;
  void exitInputMode() const;
  void executeCommand(const std::string &command);
  bool couldBeCommand(const std::string &buffer,
                      const std::vector<std::string> &commandList);
  void editorOpen(const char *fileName);

private:
  void editorAppendRow(const std::string &row);
  std::ifstream inFile;
  void goToBeginningOfLine();
  void goToTheEndOfLine();
  std::unordered_map<std::string, CommandHandler> commandHandlers;
  void registerCommand(const std::string &command, CommandHandler handler);
  std::string buffer;
  void editorDrawRows();
  int getCursorPosition();
  void scrollUp();
  void scrollDown();
  void adjustRowOffset();
  void appendRow(const std::string &row);
};
