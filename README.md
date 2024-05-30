# Panavim

This project is a terminal-based text editor written in C++. It supports basic text editing features, including navigation, text manipulation, and command execution. The editor is designed to operate in three modes: Normal, Input, and Command.

## Features

- **Basic Text Editing:** Insert, delete, and navigate through text.
- **File Operations:** Open and save files.
- **Command Mode:** Execute commands to manipulate text and navigate.
- **Search Functionality:** Search within the text and navigate through search results.

## Compilation and Execution

To compile and run the text editor, use the following commands:

```bash
g++ -o terminal_editor main.cpp Terminal.cpp
./terminal_editor [filename]
```

## Modes

### Normal Mode

In Normal mode, you can navigate and perform basic operations. Use the following keys to navigate:

- Arrow Keys or h, j, k, l move the cursor left, down, up, and right, respectively.
- i: Enter Input mode.
- : Enter Command mode.

### Input Mode

In Input mode, you can insert text. Press `ESC` to return to Normal mode.

### Command Mode

In Command mode, you can execute various commands by typing them after the `:` prompt and pressing Enter. Some commands include:

- `:w` - Save the file.
- `:q` - Quit the editor (will prompt if there are unsaved changes).
- `:wq` - Save the file and quit.
- `:q!` - Quit without saving.

## Commands

In Normal mode, you can use the following commands:

- `n` - Find next search result.
- `N` - Find previous search result.
- `gg` - Scroll to the top of the file.
- `G` - Scroll to the bottom of the file.
- `_` - Go to the beginning of the line.
- `$` - Go to the end of the line.
- `w` - Move cursor a word forward.
- `b` - Move cursor a word backward.
- `}` - Move cursor to the next empty line.
- `{` - Move cursor to the previous empty line.
- `/` - Search for a string.
- `A` - Enter Input mode at the end of the line.
- `I` - Enter Input mode at the beginning of the line.
- `o` - Insert a new row below the current row and enter Input mode.
- `O` - Insert a new row above the current row and enter Input mode.## Special Keys

- **ESC:** Exit Input or Command mode.
- **Ctrl-H or Backspace:** Delete character in Input mode.
- **DEL:** Delete character under the cursor.

## File Operations

- **Open a File:** When starting the editor, provide a filename as an argument to open it.
- **Save a File:** In Command mode, use `:w` to save the current file. If the file is new, you'll be prompted to provide a filename.
- **Save As:** When saving a new file, you'll be prompted with "Save as: " to provide a filename.

## Search Functionality

- **Search:** Press `/` in Normal mode to enter a search query.
- **Find Next:** Use `n` to navigate to the next search result.
- **Find Previous:** Use `N` to navigate to the previous search result.

## Status Bar

The status bar displays the current filename, the number of lines, and whether the file has been modified.

## Additional Notes

- **Terminal Configuration:** The editor modifies the terminal configuration to provide a smoother experience. It restores the original configuration upon exit.
- **Screen Size:** The editor adjusts to the terminal window size.

## Example Usage

1.  Start the editor with a file:

    bash

    Copy code

    `./terminal_editor example.txt`

2.  Enter Input mode by pressing `i` and start typing.
3.  Press `ESC` to return to Normal mode.
4.  Save the file by entering `:` followed by `w` and pressing Enter.
5.  Quit the editor by entering `:` followed by `q` and pressing Enter.
