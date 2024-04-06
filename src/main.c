#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "FileBuffer.h"

#define MAX(a, b) ((a > b) ? (a) : (b))
#define MIN(a, b) ((a < b) ? (a) : (b))
#define CLAMP(v, min, max) (MIN(MAX(v, min), max))
#define BIT_ISSET(var, pos) (((var) & ((long)1 << (pos))) != 0 ? 1 : 0)
#define ABS(a) ((a) > 0 ? (a) : (-a))

struct {
  int cx;
  int cy;
  int px;
  int py;
  int kill;
  char statusLine0[256];
  char statusLine1[256];
  char *filename;
} state;

struct FileBuffer *buffer;
struct winsize window;
const int statusLines = 2;
const int lnoffset = 6;
int statusChangeDelay = 0;

void resetStatusLine1();
void applyCursorPos();
void repositionCursor();
long getkey();
int getLineLen(int y);
bool ischar(int key);
void parseInput(long keyvar);
void update(SCREEN *screen, struct FileBuffer *buffer,
            const struct winsize *winsize);
void save();
void createFile();

int main(int argc, char *argv[]) {
  SCREEN *term = newterm(getenv("TERM"), stdout, stdin);
  noecho();

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &window);

  if (argc >= 2) {
    // open file at argv[0]
    // printf("open file %s\n========\n", argv[1]);
    const char *filename = argv[1];
    FILE *file = fopen(filename, "r");
    if (file != NULL) {
      fCreateBuffer(&buffer, file);
      fclose(file);
      snprintf(state.statusLine1, 256, "%s", filename);
      state.filename = malloc(sizeof(char) * strlen(filename));
      strncpy(state.filename, filename, strlen(filename));
    } else {
      cCreateBuffer(&buffer);
    }
  } else {
    // printf("create new file\n========\n");
    cCreateBuffer(&buffer);
    snprintf(state.statusLine1, 256, "No file.");
  }

  state.kill = 0;

  clear();
  refresh();
  raw();

  long key = 0;
  while (1) {
    update(term, buffer, &window);
    resetStatusLine1();
    key = getkey();
    // printf("%ld\n", key);
    parseInput(key);
    if (state.kill == 1) {
      break;
    }
  }

  endwin();
  // printBuffer(buffer);
  releaseBuffer(buffer);

  if (state.filename != NULL) {
    free(state.filename);
  }

  return 0;
}

void resetStatusLine1() {
  if (state.filename == NULL) {
    const char *lbl = "nofile";
    strncpy(state.statusLine1, lbl, 256);
  } else {
    const char *lbl = state.filename;
    strncpy(state.statusLine1, lbl, 256);
  }
}

bool ischar(int key) { return !iscntrl(key); }

bool isarrow(int key) {
  return key == 65 || key == 68 || key == 67 || key == 66;
}

long getkey() {
  int k = getch();
  long ret = 0;
  if (k == 27) {
    ret = (long)1 << 32;
    int pkey = k;
    int safety = 100;
    while (!isarrow(k)) {
      k = getch();
      if (k == -1) {
        snprintf(state.statusLine0, 256, "key error. pkey = %d", pkey);
        break;
      }
      safety--;
      if (safety <= 0) {
        snprintf(state.statusLine0, 256, "key error - safety. pkey = %d", pkey);
        k = -1;
        break;
      }
    }
    return ret | k;
  } else
    return k;
}

void update(SCREEN *screen, struct FileBuffer *buffer,
            const struct winsize *winsize) {
  clear();

  applyCursorPos();

  char lineNumber[5] = "0000\0";

  int start = state.py;
  int len = MIN(buffer->len - start, window.ws_row - 2);
  int end = start + len;

  for (int i = start, y = 0; i < end; i++, y++) {
    snprintf(lineNumber, 5, "%.4d", i + 1);
    mvaddnstr(y, 0, lineNumber, 4);
    mvaddnstr(y, lnoffset, buffer->lines[i].chars, buffer->lines[i].len);
  }

  mvaddstr(winsize->ws_row - 2, 0, state.statusLine0);
  mvaddstr(winsize->ws_row - 1, 0, state.statusLine1);

  repositionCursor();

  refresh();
}

void parseInput(long keyvar) {
  int special = BIT_ISSET(keyvar, 32);
  int key = keyvar;

  int lineRow = state.py + state.cy;
  int lineCol = state.px + state.cx - lnoffset;

  if (special) {
    switch (key) {
    case 65: // arrow up
      state.cy -= 1;
      break;
    case 68: // arrow left
      state.cx -= 1;
      break;
    case 67: // arrow right
      state.cx += 1;
      break;
    case 66: // arrow down
      state.cy += 1;
      break;
    default:
      snprintf(state.statusLine0, 256, "Unhandled special character %d [%c]",
               key, key);
      break;
    }

  } else {
    switch (key) {
    case -1:
      break;
    case 17: // ctrl+q
    case 3:  // ctrl+c
      state.kill = 1;
      break;
    case 19: // ctrl+s
      // save!
      if (state.filename == NULL) {
        createFile();
      } else {
        save();
        snprintf(state.statusLine1, 256, "File saved.");
      }
      break;
    case 10: //<RETURN>
      if (lineCol == 0) {
        insertLine(lineRow, buffer);
      } else {
        splitAndInsertLine(lineCol, lineRow, buffer);
        int llnxt = getLineLen(lineRow + 1) + lnoffset - 1;
        state.cx = llnxt;
      }
      state.cy += 1;
      break;
    case 127: // backspace
    {
      int llprv = getLineLen(lineRow - 1) + lnoffset;
      removeFromBuffer(lineCol - 1, lineRow, buffer);
      state.cx--;
      if (state.cx - lnoffset < 0) {
        state.cx = llprv;
        state.cy--;
      }
      break;
    }
    // case 9:
    //		 placeInBuffer(state.cx - lnoffset, state.cy, '\t',
    // buffer); 		 state.cx++; 		 break;
    default:
      if (ischar(key)) {
        placeInBuffer(lineCol, lineRow, key, buffer);
        state.cx++;
        snprintf(state.statusLine0, 256, "write text %c", key);
      } else {
        snprintf(state.statusLine0, 256, "input error: %d is not a key [%c]",
                 key, key);
      }
      break;
    }
  }
}

void applyCursorPos() {
  int pagelen = window.ws_row - 2;
  if (state.cy >= pagelen) {
    int toofar = state.cy - pagelen + 1;
    state.py += toofar;
    state.cy -= toofar;
  } else if (state.cy < 0 && state.py > 0) {
    state.py -= ABS(state.cy);
    state.cy = 0;
  }
}
void repositionCursor() {

  int end = MIN(buffer->len - state.py, window.ws_row - 2 - 1);
  state.cy = CLAMP(state.cy, 0, end);
  int linelen =
      state.cy >= buffer->len ? 0 : buffer->lines[state.cy + state.py].len - 1;
  linelen = MAX(linelen, 0);
  state.cx = CLAMP(state.cx, lnoffset, linelen + lnoffset);
  move(state.cy, state.cx);
}

int getLineLen(int y) {
  int linelen = y >= buffer->len ? 0 : buffer->lines[y].len - 1;
  linelen = MAX(linelen, 0);
  return linelen;
}

void createFile() {}

void save() {
  FILE *f = fopen(state.filename, "w");
  for (int i = 0; i < buffer->len; i++) {
    struct Line *l = &buffer->lines[i];
    if (l->len == 0) {
      fprintf(f, "\n");
    } else {
      fprintf(f, "%s\n", l->chars);
    }
  }
  fclose(f);
}
