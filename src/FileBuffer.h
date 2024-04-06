#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Line {
  char *chars;
  size_t len;
};

struct FileBuffer {
  struct Line *lines;
  size_t len;
};

void allocLine(struct FileBuffer *f);
void insertLine(int y, struct FileBuffer *b);
void removeLines(struct FileBuffer *f, int y, int count);
void splitAndInsertLine(int x, int y, struct FileBuffer *b);
void expandLine(struct Line *l, int by);
void subtractLine(struct Line *l, int by);
void appendChar(struct Line *l, char c);
void insertChar(struct Line *l, char c, int at);

char *sclone(const char str[], int len, int it);

void fCreateBuffer(struct FileBuffer **f, FILE *file);
void cCreateBuffer(struct FileBuffer **f);
void releaseBuffer(struct FileBuffer *f);
void printBuffer(struct FileBuffer *f);
void placeInBuffer(int x, int y, char c, struct FileBuffer *b);
void removeFromBuffer(int x, int y, struct FileBuffer *b);
