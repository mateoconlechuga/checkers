#ifndef SIMPLE_CHECKERS_H
#define SIMPLE_CHECKERS_H

#define OCCUPIED 0
#define WHITE 1
#define BLACK 2
#define MAN 4
#define KING 8
#define FREE 16

/* return values */
#define DRAW 0
#define WIN 1
#define LOSS 2
#define UNKNOWN 3

void getmove(uint8_t b[8][8], uint8_t color, int *playnow);

#endif
