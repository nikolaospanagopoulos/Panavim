#pragma once
#include <ctime>
#include <fstream>
#include <functional>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <unordered_map>

struct Row {
  std::string textRow;
  std::string renderedRow;
  Row();
  Row(const std::string textRow, const std::string renderedRow);
};

class Terminal {
public:
  enum editorKey { ARROW_LEFT = 1000, ARROW_RIGHT, ARROW_UP, ARROW_DOWN };
  enum MODE { NORMAL, INPUT, COMMAND }; // Added NORMAL mode for clarity
  enum FILE_STATUS { MODIFIED, NOT_MODIFIED };
  enum PROMPT_TYPE { SAVE_FILE, SEARCH };

  enum SPECIAL_KEYS { ESCAPE_KEY = 127, CTRL_H = 8, DEL = 1500 };
  using CommandHandler = std::function<void(Terminal &)>;
  struct editorState {
    int cx;
    int cy;

    int rx;
    int screenRows;
    int screenCols;
    struct termios originalTermios;
    MODE terminalMode;
    std::string commandBuffer;
    int numRow;
    std::vector<Row> textRows;
    int rowOffset;
    int colOffset;
    std::string fileName;
    std::string statusMsg;
    time_t statusMsgTime;
    FILE_STATUS file_status;
  } state;
  Terminal();
  ~Terminal();
  void editorRefreshScreen();
  int getWindowSize();
  void moveCursor(const int key);
  int editorCheckForKey(const char &key);
  void enterInputMode() const;
  void exitInputMode() const;
  void executeCommand(const std::string &command);
  bool couldBeCommand(const std::string &buffer,
                      const std::vector<std::string> &commandList);
  void editorOpen(const char *fileName);
  void setStatusMessage(std::string msg);
  void handleCharForInputMode(int c);
  void editorSave();
  void editorDeleteChar();
  void editorInsertNewLineAt(unsigned long at);

  using promptCallbackFunc = std::function<void(const std::string &)>;
  std::vector<std::pair<int, int>>
      searchResults;            // Vector to store search results (row, col)
  int currentSearchResultIndex; // Index to track the current search result
  std::string lastSearchQuery;  // Store the last search query
  int originalCx, originalCy;

  std::string prompt(const std::string &message, PROMPT_TYPE promptType,
                     promptCallbackFunc func);

  void handlePromptInput(int c, std::string &inputBuffer, bool &promptActive,
                         int &cursorPosition, PROMPT_TYPE promptType,
                         promptCallbackFunc func = nullptr);
  void find();
  void findPrevious();
  void findNext();
  void performSearch(const std::string &query);

private:
  void editorRowInsertChar(Row &row, int at, int c);
  void editorUpdateRow(Row &row);
  std::ifstream inFile;
  std::ofstream outFile;
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
  void insertRow(int at, const std::string line);
  int cxTorx(Row &row, int cx);
  void moveCursorAwordForward();
  void moveCursorAwordBackwards();
  void drawStatusBar();
  void moveCursorToNextLineWithSpace();
  void moveCursorToPrevLineWithSpace();
  void drawMessageCommandBar();
  void insertChar(int c);
  std::string rowsToFinalStr(long *const sizePtr);
  void editorDeleteCharAt(Row &row, int at);
  void editorDeleteRow(int at);
  void editorRowAppendString(Row &row, std::string &toAppend);
  void editorInsertNewLine();
};
