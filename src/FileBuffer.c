#include "FileBuffer.h"

#include <stdlib.h>
#include <string.h>

void allocLine(struct FileBuffer *f) {

    if (f->len == 0) {
        f->len = 1;
        struct Line *lines = malloc(f->len * sizeof(struct Line));
        lines[0].len = 0;
        f->lines = lines;
    } else {
        f->len++;
        struct Line *newline = realloc(f->lines, f->len * sizeof(struct Line));
        if (newline == NULL) {
            printf("REALLOC FAILED \n");
            f->len--;
            return;
        }
        f->lines = newline;
        f->lines[f->len - 1].len = 0;
    }
}

void insertLine(int y, struct FileBuffer *f) {
    // todo lol
    if (y > f->len)
        return;
    if (y == f->len) {
        allocLine(f);
    } else {
        f->len++;
        struct Line *newline = realloc(f->lines, f->len * sizeof(struct Line));
        if (newline == NULL) {
            printf("REALLOC FAILED \n");
            f->len--;
            return;
        }
        f->lines = newline;
        int len = f->len - y;
        memmove(f->lines + y + 1, f->lines + y, len * sizeof(struct Line));
        f->lines[y].len = 0;
        f->lines[y].chars = NULL;
    }
}

void removeLines(struct FileBuffer *f, int y, int count) {
    if (count > f->len - y) {
        count = f->len - y;
    }
    int len = f->len - y - count;
    memmove(f->lines + y, f->lines + y + count, len * sizeof(struct Line));
    f->len -= count;
    struct Line *newline = realloc(f->lines, f->len * sizeof(struct Line));
    // if this fails there's no way back lol XD
    // if (newline == NULL) {
    //   printf("REALLOC FAILED \n");
    //   f->len += count;
    //   return;
    // }
    f->lines = newline;
}

void splitAndInsertLine(int x, int y, struct FileBuffer *f) {
    // todo lol
    y++;
    if (y > f->len)
        return;
    // if (y == f->len) {
    //   allocLine(f);
    // }
    f->len++;
    struct Line *newline = realloc(f->lines, f->len * sizeof(struct Line));
    if (newline == NULL) {
        printf("REALLOC FAILED \n");
        f->len--;
        return;
    }
    f->lines = newline;
    int len = f->len - y;
    memmove(f->lines + y + 1, f->lines + y, len * sizeof(struct Line));

    struct Line *lnew = &f->lines[y];
    lnew->len = 0;
    lnew->chars = NULL;
    // split now

    struct Line *lprv = &f->lines[y - 1];
    int toSplit = strlen(lprv->chars) - x;
    if (toSplit <= 0)
        return;

    char *contentClone = sclone(lprv->chars, lprv->len, 0);
    int newLineLen = lprv->len - x;
    subtractLine(lprv, newLineLen - 1);

    expandLine(lnew, newLineLen);
    strncpy(lnew->chars, contentClone + x, newLineLen);

    free(contentClone);
}

void expandLine(struct Line *l, int by) {
    if (by <= 0)
        return;

    if (l->len == 0) {
        l->len = by + 1;
        char *chars = malloc(l->len * sizeof(char));
        l->chars = chars;
    } else {
        l->len += by;
        char *newchar = realloc(l->chars, l->len * sizeof(char));
        if (newchar == NULL) {
            printf("REALLOC FAILED \n");
            l->len -= by;
            return;
        }
        l->chars = newchar;
    }
}

void subtractLine(struct Line *l, int by) {
    if (by <= 0)
        return;
    if (by > l->len)
        return;

    if (by == l->len) {
        // clear line
        free(l->chars);
        l->len = 0;
        return;
    }

    char *newchar = realloc(l->chars, (l->len - by) * sizeof(char));
    if (newchar == NULL) {
        printf("REALLOC FAILED\n");
        return;
    }
    l->chars = newchar;
    l->len -= by;
    l->chars[l->len - 1] = '\0';
}

void appendChar(struct Line *l, char c) {
    if (c == '\n')
        return;
    expandLine(l, 1);
    l->chars[l->len - 2] = c;
    l->chars[l->len - 1] = '\0';
}

void insertChar(struct Line *l, char c, int at) {
    if (l->len < at)
        return; // cant insert char

    if (at == 0) {
        // insert at start
        char *clonestr = sclone(l->chars, l->len, 0);
        expandLine(l, 1);
        l->chars[0] = c;
        l->chars[1] = '\0';
        strcat(l->chars, clonestr);
        free(clonestr);
    } else if (at == l->len) {
        // append
        appendChar(l, c);
    } else {
        // insert arbitrarily
        expandLine(l, 1);
        int len = strlen(l->chars) - at;
        memmove(l->chars + at + 1, l->chars + at, sizeof(char) * len);
        l->chars[at] = c;
    }
}

char *sclone(const char str[], int len, int spacers) {
    if (spacers == 0) {
        char *strcp = malloc(sizeof(char) * len);
        strncpy(strcp, str, len);
        return strcp;
    } else {
        char *strcp = malloc(sizeof(char) * (len + spacers));
        strncpy(strcp, str, len);
        strcp[len + spacers - 1] = '\0';
        return strcp;
    }
}

void fCreateBuffer(struct FileBuffer **f, FILE *file) {
    struct FileBuffer *nf = malloc(sizeof(struct FileBuffer));
    nf->len = 0;
    nf->lines = NULL;
    *f = nf;

    int lit = 0;

    for (char c = fgetc(file); c != EOF; c = fgetc(file)) {
        // printf("%d\n", c);
        if (lit >= (*f)->len) {
            allocLine(*f);
        }

        appendChar(&(*f)->lines[lit], c);
        if (c == '\n') {
            // new line
            lit++;
        }
    }
}

void cCreateBuffer(struct FileBuffer **f) {
    struct FileBuffer *nf = malloc(sizeof(struct FileBuffer));
    nf->len = 0;
    nf->lines = NULL;
    *f = nf;
}

void releaseBuffer(struct FileBuffer *f) {
    printf("Freeing buffer.\n");
    if (f->len > 0) {
        for (int i = 0; i < f->len; i++) {
            if (f->lines[i].len <= 0)
                continue;
            free(f->lines[i].chars);
        }
        free(f->lines);
    }
    free(f);
}

void printBuffer(struct FileBuffer *f) {
    if (f == NULL)
        return;

    for (int i = 0; i < f->len; i++) {
        printf("%s\n", f->lines[i].chars);
    }
}

void placeInBuffer(int x, int y, char c, struct FileBuffer *b) {
    if (y > b->len)
        return; // too far
    if (y == b->len) {
        allocLine(b);
    }
    struct Line *l = &b->lines[y];
    if (l->len < x)
        return;
    insertChar(l, c, x);
}

void removeFromBuffer(int x, int y, struct FileBuffer *b) {
    if (y >= b->len)
        return;

    struct Line *l = &b->lines[y];
    int belowZero = 0;
    if (x < 0) {
        x = 0;
        belowZero = 1;
    }
    if (l->len < x)
        return;

    if (y <= 0 && x <= 0)
        return;

    if (l->len == 0 || strlen(l->chars) == 0) {
        removeLines(b, y, 1);
    } else {
        if (belowZero) {
            expandLine(&b->lines[y - 1], strlen(l->chars));
            struct Line *prvLine = &b->lines[y - 1];
            strncat(prvLine->chars, b->lines[y].chars, b->lines[y].len);
            removeLines(b, y, 1);
        } else {
            int len = strlen(l->chars) - x - 1;
            memmove(l->chars + x, l->chars + x + 1, sizeof(char) * len);
            subtractLine(l, 1);
        }
    }
}
