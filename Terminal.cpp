#include "Terminal.hpp"
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#define EditorVersion 1

Row::Row() : textRow{}, renderedRow{} {}
Row::Row(const std::string row, const std::string renderedRow)
    : textRow{row}, renderedRow{} {}

int Terminal::cxTorx(Row &row, int cx) {
  int rx = 0;
  for (int j = 0; j < cx; j++) {
    if (row.textRow.at(j) == '\t') {
      rx += (8 - 1) - (rx % 8);
    }
    rx++;
  }
  return rx;
}

void Terminal::handleCharForInputMode(int c) {

  switch (c) {
  case ARROW_UP:
    moveCursor('k');
    break;
  case ARROW_DOWN:
    moveCursor('j');
    break;
  case ARROW_LEFT:
    moveCursor('h');
    break;
  case ARROW_RIGHT:
    moveCursor('l');
    break;
  case '\r':
    editorInsertNewLine();
    break;
  case DEL:
    moveCursor('l');
    editorDeleteChar();
    break;
  case ESCAPE_KEY:
  case CTRL_H:
    editorDeleteChar();
    break;
  default:
    insertChar(c);
  }
}

std::string Terminal::rowsToFinalStr(long *const sizePtr) {
  std::string buf;
  long count = 0;
  for (const Row &row : state.textRows) {
    buf.append(row.textRow + '\n');
    count += row.textRow.size() + 1;
  }
  *sizePtr = count;
  return buf;
}

void Terminal::editorSave() {

  long bufferSize = 0;
  if (state.fileName.empty()) {
    return;
  }
  outFile.open(state.fileName);
  std::string toSaveLines = rowsToFinalStr(&bufferSize);
  outFile << toSaveLines;
  std::string message = std::to_string(bufferSize) + " bytes written to disk";
  setStatusMessage(message);
  state.file_status = NOT_MODIFIED;
}

void Terminal::insertChar(int c) {
  if (state.cy == state.numRow) {
    std::string newRow = "";
    insertRow(0, newRow);
  }
  editorRowInsertChar(state.textRows[state.cy], state.cx, c);
  state.cx++;
  state.file_status = MODIFIED;
}
void Terminal::editorRowInsertChar(Row &row, int at, int c) {
  if (at < 0 || at > row.textRow.size()) {
    at = row.textRow.size();
  }
  row.textRow.insert(at, 1, c);
  editorUpdateRow(row);
}
void Terminal::adjustRowOffset() {
  state.rx = 0;

  if (state.cy < state.numRow) {
    state.rx = cxTorx(state.textRows.at(state.cy), state.cx);
  }

  if (state.cy < state.rowOffset) {
    state.rowOffset = state.cy;
  }
  if (state.cy >= state.rowOffset + state.screenRows) {
    state.rowOffset = state.cy - state.screenRows + 1;
  }
  if (state.rx < state.colOffset) {
    state.colOffset = state.rx;
  }
  if (state.rx >= state.colOffset + state.screenCols) {
    state.colOffset = state.rx - state.screenCols + 1;
  }
}

void Terminal::editorUpdateRow(Row &row) {
  row.renderedRow.clear();
  int tabsCount = 0;
  for (const char &c : row.textRow) {
    if (c == '\t') {
      tabsCount++;
    }
  }
  // each tab counted as one but we need 8 chars for a tab. multiplied by 7
  // because textrow size already counts 1
  row.renderedRow.resize(row.textRow.size() + tabsCount * 7 + 1);
  int index = 0;
  for (const char &c : row.textRow) {
    if (c == '\t') {
      row.renderedRow[index++] = ' ';
      while (index % 8 != 0) {
        row.renderedRow[index++] = ' ';
      }
    } else {
      row.renderedRow[index++] = c;
    }
  }
}

void Terminal::insertRow(int at, const std::string &line) {

  if (at < 0 || at > state.numRow) {
    return;
  }

  Row row = Row{line, ""};

  editorUpdateRow(row);
  state.textRows.insert(state.textRows.begin() + at, std::move(row));
  state.numRow++;
  state.file_status = MODIFIED;
}

void Terminal::editorOpen(const char *fileName) {
  inFile.open(fileName);
  this->state.fileName = fileName;
  if (!inFile) {
    throw std::runtime_error("failed to get open file");
  }
  std::string line = {};
  while (std::getline(inFile, line)) {
    // put the string in the vector. don't copy it
    // state.textRows.emplace_back(std::move(line));
    insertRow(state.numRow, line);
  }
  state.file_status = NOT_MODIFIED;
}

void Terminal::registerCommand(const std::string &command,
                               CommandHandler handler) {
  commandHandlers[command] = handler;
}

void Terminal::executeCommand(const std::string &command) {
  auto it = commandHandlers.find(command);
  if (it != commandHandlers.end()) {
    it->second(*this);
    state.commandBuffer.clear();
  }
}

void Terminal::editorDeleteChar() {
  if (state.cy == state.numRow && state.terminalMode != COMMAND) {
    return;
  }
  if (state.cx == 0 && state.cy == 0) {
    return;
  }
  Row &row = state.textRows.at(state.cy);
  if (state.cx > 0) {
    editorDeleteCharAt(row, state.cx - 1);
    state.cx--;
  } else {
    state.cx = state.textRows.at(state.cy - 1).textRow.size();
    editorRowAppendString(state.textRows.at(state.cy - 1), row.textRow);
    editorDeleteRow(state.cy);
    state.cy--;
  }
}

void Terminal::editorDeleteCharAt(Row &row, int at) {
  if (at < 0 || at >= row.textRow.size()) {
    return;
  }
  row.textRow.erase(row.textRow.begin() + at);
  editorUpdateRow(row);
  state.file_status = MODIFIED;
}

bool Terminal::couldBeCommand(const std::string &buffer,
                              const std::vector<std::string> &commandList) {
  for (const auto &cmd : commandList) {
    if (cmd.rfind(buffer, 0) == 0) {
      return true;
    }
  }
  return false;
}

void Terminal::goToTheEndOfLine() {
  if (state.cy < state.numRow) {
    state.cx = state.textRows[state.cy].textRow.size();
  }
}
void Terminal::drawStatusBar() {
  buffer.append("\x1b[7m", 4);
  std::stringstream statusStream{};
  statusStream << state.fileName ? state.fileName : "[No Name]";
  statusStream << " - lines " << state.numRow;
  statusStream << ((state.file_status == MODIFIED) ? " (modified)" : "");
  std::string status = statusStream.str();

  int len = status.size();
  if (len > state.screenCols) {
    len = state.screenCols;
  }
  buffer.append(status.c_str(), len);
  while (len < state.screenCols) {
    buffer.append(" ");
    len++;
  }
  buffer.append("\x1b[m");
  buffer.append("\r\n");
}
void Terminal::goToBeginningOfLine() { state.cx = 0; }

void Terminal::setStatusMessage(std::string msg) {
  state.statusMsg.clear();
  state.statusMsg = msg;
  state.statusMsgTime = time(NULL);
}

void Terminal::drawMessageCommandBar() {
  buffer.append("\x1b[K");
  int msgLen = state.statusMsg.size();
  if (msgLen > state.screenCols) {
    msgLen = state.screenCols;
  }
  if (msgLen && time(NULL) - state.statusMsgTime < 5) {

    buffer.append(state.statusMsg.c_str(), msgLen);
  }
}

Terminal::Terminal()
    : state({.cx = 0,
             .cy = 0,
             .rx = 0,
             .screenRows = 0,
             .screenCols = 0,
             .terminalMode = NORMAL,
             .commandBuffer = {},
             .numRow = 0,
             .textRows = {},
             .rowOffset = 0,
             .colOffset = 0,
             .fileName = {},
             .statusMsg = {""},
             .statusMsgTime = 0,
             .file_status = NOT_MODIFIED}),

      buffer{""}, inFile{}, outFile{} {
  // get current terminal options
  if (tcgetattr(STDIN_FILENO, &state.originalTermios) == -1) {
    throw std::runtime_error("failed to get current terminal attributes");
  }
  getWindowSize();
  state.screenRows -= 2;

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
  registerCommand("gg", [](Terminal &term) { term.scrollUp(); });
  registerCommand("G", [](Terminal &term) { term.scrollDown(); });
  registerCommand("_", [](Terminal &term) { term.goToBeginningOfLine(); });
  registerCommand("$", [](Terminal &term) { term.goToTheEndOfLine(); });
  registerCommand("w", [](Terminal &term) { term.moveCursorAwordForward(); });
  registerCommand("b", [](Terminal &term) { term.moveCursorAwordBackwards(); });
  registerCommand("}",
                  [](Terminal &term) { term.moveCursorToNextLineWithSpace(); });
  registerCommand("{",
                  [](Terminal &term) { term.moveCursorToPrevLineWithSpace(); });
}
void Terminal::moveCursorToPrevLineWithSpace() {
  while (state.cy > 0) {
    state.cy--;
    if (state.cy <= 0) {
      state.cy = 0;
      break;
    }
    if (state.textRows.at(state.cy).textRow.empty()) {
      state.cx = 0;
      break;
    }
  }
}

void Terminal::moveCursorToNextLineWithSpace() {
  while (state.cy < state.numRow) {
    state.cy++;
    if (state.cy >= state.numRow) {
      state.cy = state.numRow - 1;
      break;
    }
    if (state.textRows.at(state.cy).textRow.empty()) {
      state.cx = 0;
      break;
    }
  }
}

void Terminal::moveCursorAwordBackwards() {

  if (state.cy > 0) {

    if (state.cx == 0) {
      state.cy--;
      state.cx = state.textRows.at(state.cy).textRow.size();
      return;
    }
  }
  while (state.cx > 0 &&
         state.cx <= state.textRows.at(state.cy).textRow.size()) {
    char prev = state.textRows.at(state.cy).textRow[state.cx];
    if (!std::isspace(state.textRows.at(state.cy).textRow[state.cx]) &&
        !std::isupper(state.textRows.at(state.cy).textRow[state.cx])) {
      state.cx--;
    } else {
      if (std::isspace(prev)) {
        state.cx--;
        if (!std::isspace(state.textRows.at(state.cy).textRow[state.cx])) {
          break;
        }
        continue;
      }
      state.cx--;
      break;
    }
  }
}

void Terminal::moveCursorAwordForward() {
  if (state.cy < state.numRow) {

    if (state.cx == state.textRows.at(state.cy).textRow.size()) {
      state.cy++;
      state.cx = 0;
      return;
    }
    while (state.cx < state.textRows.at(state.cy).textRow.size()) {
      char prev = state.textRows.at(state.cy).textRow[state.cx];
      if (!std::isspace(state.textRows.at(state.cy).textRow[state.cx]) &&
          !std::isupper(state.textRows.at(state.cy).textRow[state.cx])) {
        state.cx++;
      } else {
        if (std::isspace(prev)) {
          state.cx++;
          if (!std::isspace(state.textRows.at(state.cy).textRow[state.cx])) {
            break;
          }
          continue;
        }
        state.cx++;
        break;
      }
    }
  }
}

void Terminal::scrollUp() {
  state.cy = state.rowOffset;
  int times = state.screenRows;
  while (times--) {
    moveCursor('k');
  }
}
void Terminal::scrollDown() {

  state.cy = state.rowOffset + state.screenRows - 1;
  if (state.cy > state.numRow) {
    state.cy = state.numRow;
  }
  int times = state.screenRows;
  while (times--) {
    moveCursor('j');
  }
}
int Terminal::editorCheckForKey(const char &key) {
  if (key == '\x1b') {
    char seq[3]{};
    if (read(STDIN_FILENO, &seq[0], 1) != 1) {
      return '\x1b';
    }
    if (read(STDIN_FILENO, &seq[1], 1) != 1) {
      return '\x1b';
    }
    if (seq[0] == '[') {
      switch (seq[1]) {
      case 'A':
        return ARROW_UP;
      case 'B':
        return ARROW_DOWN;
      case 'C':
        return ARROW_RIGHT;
      case 'D':
        return ARROW_LEFT;
      case '3':
        if (read(STDIN_FILENO, &seq[2], 1) != 1) {
          return '\x1b';
        }
        if (seq[2] == '~') {
          return 1500;
        }
        break;
      }
    }
    return '\x1b';
  } else {
    return key;
  }
}

void Terminal::moveCursor(const int key) {

  // check if on an actual line
  std::string *editorRow = (state.cy >= state.numRow)
                               ? nullptr
                               : &state.textRows.at(state.cy).textRow;
  switch (key) {
  case editorKey::ARROW_DOWN:
  case 'j':
    if (state.cy < state.numRow) {
      state.cy++;
    }
    break;
  case editorKey::ARROW_UP:
  case 'k':
    if (state.cy != 0) {
      state.cy--;
    }
    break;
  case editorKey::ARROW_RIGHT:
  case 'l':
    if (editorRow && state.cx < editorRow->size()) {
      state.cx++;
    } else if (editorRow && state.cx == editorRow->size()) {
      state.cy++;
      state.cx = 0;
    }
    break;
  case editorKey::ARROW_LEFT:
  case 'h':
    if (state.cx != 0) {
      state.cx--;
    } else if (state.cy > 0) {
      state.cy--;
      state.cx = state.textRows.at(state.cy).textRow.size();
    }
    break;
  }
  editorRow = (state.cy >= state.numRow) ? nullptr
                                         : &state.textRows.at(state.cy).textRow;
  int rowSize = editorRow ? editorRow->size() : 0;
  if (state.cx > rowSize) {
    state.cx = rowSize;
  }
}
void Terminal::exitInputMode() const { std::cout << "\x1b[2 q" << std::flush; }
void Terminal::editorRefreshScreen() {

  adjustRowOffset();

  // l comman and argument 25 used to hide cursor
  buffer.append("\x1b[?25l");
  // K command -> erase each line
  // H command -> reposition cursor at top left. default argument 1
  buffer.append("\x1b[H");
  editorDrawRows();
  drawStatusBar();

  drawMessageCommandBar();
  // H command -> reposition cursor
  std::stringstream cursorStream{};

  cursorStream << "\x1b[" << (state.cy - state.rowOffset) + 1 << ";"
               << (state.rx - state.colOffset) + 1 << "H";

  buffer.append(std::move(cursorStream.str()));
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
    int filerow = y + state.rowOffset;
    if (filerow >= state.numRow) {

      if (state.numRow == 0 && y == state.screenRows / 3) {
        // put welcomeMessage in center
        // devide screen width by two and then subtract half the string length
        // to know where to start writing add spaces for the rest
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
    } else {
      int len = state.textRows.at(filerow).renderedRow.size() - state.colOffset;
      if (len < 0) {
        len = 0;
      }
      if (len > state.screenCols) {
        len = state.screenCols;
      }
      buffer.append(&state.textRows.at(filerow).renderedRow[state.colOffset],
                    len);
    }
    buffer.append("\x1b[K");
    buffer.append("\r\n");
  }
}

void Terminal::enterInputMode() const { std::cout << "\x1b[6 q" << std::flush; }
Terminal::~Terminal() {
  // restore old terminal config
  buffer.clear();
  // close file
  if (inFile) {
    inFile.close();
  }
  if (outFile) {
    outFile.close();
  }
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &state.originalTermios);
  // clear screen
  std::cout << "\x1b[2J" << std::flush;
  // Move cursor to home position
  std::cout << "\x1b[H" << std::flush;
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
void Terminal::editorDeleteRow(int at) {
  if (at < 0 || at >= state.numRow)
    return;
  state.textRows.erase(state.textRows.begin() + at);
  state.numRow--;
  state.file_status = MODIFIED;
}
void Terminal::editorRowAppendString(Row &row, std::string &toAppend) {
  row.textRow += std::move(toAppend);
  editorUpdateRow(row);
  state.file_status = MODIFIED;
}
void Terminal::editorInsertNewLineAt(unsigned long at) { insertRow(at, ""); }

void Terminal::editorInsertNewLine() {
  if (state.cx == 0) {
    editorInsertNewLineAt(0);
  } else {
    Row &row = state.textRows.at(state.cy);

    std::string seperated =
        row.textRow.substr(state.cx, row.textRow.size() - state.cx);
    row.textRow.erase(state.cx, state.textRows.size() - state.cx);
    insertRow(state.cy + 1, seperated);
    row = state.textRows.at(state.cy);
    editorUpdateRow(row);
  }
  state.cy++;
  state.cx = 0;
}
