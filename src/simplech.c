/**
 * Simple Checkers Engine improved and modified by Matt Waltz of use on the CE calculator
 */

/**
 * name: simple checkers with enhancements
 * author: martin fierz
 * purpose: platform independent checkers engine
 * version: 1.15
 * date: 4th february 2011
 *
 * description: simplech.c contains a simple but fast checkers engine
 * and some routines to interface to checkeriboard. simplech.c contains three
 * main parts: interface, search and move generation. these parts are
 * separated in the code.
 *
 * iboard representation: the standard checkers notation is
 *
 *      (white)
 *   32  31  30  29
 * 28  27  26  25
 *   24  23  22  21
 * 20  19  18  17
 *   16  15  14  13
 * 12  11  10   9
 *   8   7   6   5
 * 4   3   2   1
 *     (black)
 *
 * the internal representation of the iboard is different, it is a
 * array of int with length 46, the checkers iboard is numbered
 * like this:
 *
 *     (white)
 *   37  38  39  40
 * 32  33  34  35
 *   28  29  30  31
 * 23  24  25  26
 *   19  20  21  22
 * 14  15  16  17
 *   10  11  12  13
 * 5   6   7   8
 *     (black)
 *
 * let's say, you would like to teach the program that it is
 * important to keep a back rank guard. you can for instance
 * add the following (not very sophisticated) code for this:
 *
 * if(cboard[6] & (BLACK|MAN)) eval++;
 * if(cboard[8] & (BLACK|MAN)) eval++;
 * if(cboard[37] & (WHITE|MAN)) eval--;
 * if(cboard[39] & (WHITE|MAN)) eval--;
 *
 * the evaluation function is seen from the point of view of the
 * black player, so you increase the value v if you think the
 * position is good for black.
 *
 * simple checkers is free for anyone to use, in any way, explicitly also
 * in commercial products without the need for asking me. Naturally, I would
 * appreciate if you tell me that you are using it, and if you acknowledge
 * my contribution to your project.
 *
 * questions, comments, suggestions to:
 *
 * Martin Fierz
 * checkers@fierz.ch
 *
 */

/* includes */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <debug.h>
#include <tice.h>
#include <stdlib.h>

/* definitions */
#include "simplech.h"
#define CHANGECOLOR 3
#define MAXMOVES 51

/* structure definitions */
struct move2 {
    short n;
    int m[8];
};

/* used to quickly exit */
uint8_t exit_key;

/* function prototypes  */
void getmove(uint8_t cboard[8][8], uint8_t color, int *playnow);
void movetonotation(struct move2 move);

/* search */
int  checkers(uint8_t color);
int  alphabeta(int depth, int alpha, int beta, uint8_t color);
int  firstalphabeta(int depth, int alpha, int beta, uint8_t color, struct move2 *best);
void domove(struct move2 move);
void undomove(struct move2 move);
int  evaluation(uint8_t color);

/* move generation */
int  generatemovelist(struct move2 movelist[MAXMOVES], uint8_t color);
int  generatecapturelist(struct move2 movelist[MAXMOVES], uint8_t color);
void blackmancapture(int *n, struct move2 movelist[MAXMOVES], int square);
void blackkingcapture(int *n, struct move2 movelist[MAXMOVES], int square);
void whitemancapture(int *n, struct move2 movelist[MAXMOVES], int square);
void whitekingcapture(int *n, struct move2 movelist[MAXMOVES], int square);
int  testcapture(uint8_t color);

/* globals  */
int value[17] = {0, 0, 0, 0, 0, 1, 256, 0, 0, 16, 4096, 0, 0, 0, 0, 0, 0};
int *play;
uint8_t cboard[46];

#include <debug.h>

/**
 * getmove is what checkeriboard calls. you get 6 parameters:
 * cboard[8][8] 	is the current position. the values in the array are determined by
 * the #defined values of BLACK, WHITE, KING, MAN. a black king for
 * instance is represented by BLACK|KING.
 * color is the side to make a move. BLACK or WHITE.
 * maxtime is the time your program should use to make a move. this is
 * what you specify as level in checkeriboard. so if you exceed
 * this time it's not too bad - just don't exceed it too much...
 * str is a pointer to the output string of the checkeriboard status bar.
 * you can use sprintf(str,"information"); to print any information you
 * want into the status bar.
 * *playnow	is a pointer to the playnow variable of checkeriboard. if the user
 * would like your engine to play immediately, this value is nonzero,
 * else zero. you should respond to a nonzero value of *playnow by
 * interrupting your search IMMEDIATELY.
 */

void getmove(uint8_t inboard[8][8], uint8_t color, int *playnow) {
    uint8_t i;
    
    /* initialize iboard */
    for(i = 0; i < 46; i++) {
        cboard[i] = OCCUPIED;
    }
    for(i = 5; i < 41; i++) {
        cboard[i] = FREE;
    }

    cboard[5] = inboard[0][0];
    cboard[6] = inboard[2][0];
    cboard[7] = inboard[4][0];
    cboard[8] = inboard[6][0];
    cboard[10] = inboard[1][1];
    cboard[11] = inboard[3][1];
    cboard[12] = inboard[5][1];
    cboard[13] = inboard[7][1];
    cboard[14] = inboard[0][2];
    cboard[15] = inboard[2][2];
    cboard[16] = inboard[4][2];
    cboard[17] = inboard[6][2];
    cboard[19] = inboard[1][3];
    cboard[20] = inboard[3][3];
    cboard[21] = inboard[5][3];
    cboard[22] = inboard[7][3];
    cboard[23] = inboard[0][4];
    cboard[24] = inboard[2][4];
    cboard[25] = inboard[4][4];
    cboard[26] = inboard[6][4];
    cboard[28] = inboard[1][5];
    cboard[29] = inboard[3][5];
    cboard[30] = inboard[5][5];
    cboard[31] = inboard[7][5];
    cboard[32] = inboard[0][6];
    cboard[33] = inboard[2][6];
    cboard[34] = inboard[4][6];
    cboard[35] = inboard[6][6];
    cboard[37] = inboard[1][7];
    cboard[38] = inboard[3][7];
    cboard[39] = inboard[5][7];
    cboard[40] = inboard[7][7];
    for(i = 5; i <= 40; i++) {
        if(cboard[i] == 0) {
            cboard[i] = FREE;
        }
    }
    for(i = 9; i <= 36; i += 9) {
        cboard[i] = OCCUPIED;
    }

    play = playnow;
    checkers(color);
    
    /* return the iboard */
    inboard[0][0] = cboard[5];
    inboard[2][0] = cboard[6];
    inboard[4][0] = cboard[7];
    inboard[6][0] = cboard[8];
    inboard[1][1] = cboard[10];
    inboard[3][1] = cboard[11];
    inboard[5][1] = cboard[12];
    inboard[7][1] = cboard[13];
    inboard[0][2] = cboard[14];
    inboard[2][2] = cboard[15];
    inboard[4][2] = cboard[16];
    inboard[6][2] = cboard[17];
    inboard[1][3] = cboard[19];
    inboard[3][3] = cboard[20];
    inboard[5][3] = cboard[21];
    inboard[7][3] = cboard[22];
    inboard[0][4] = cboard[23];
    inboard[2][4] = cboard[24];
    inboard[4][4] = cboard[25];
    inboard[6][4] = cboard[26];
    inboard[1][5] = cboard[28];
    inboard[3][5] = cboard[29];
    inboard[5][5] = cboard[30];
    inboard[7][5] = cboard[31];
    inboard[0][6] = cboard[32];
    inboard[2][6] = cboard[33];
    inboard[4][6] = cboard[34];
    inboard[6][6] = cboard[35];
    inboard[1][7] = cboard[37];
    inboard[3][7] = cboard[38];
    inboard[5][7] = cboard[39];
    inboard[7][7] = cboard[40];
}


void movetonotation(struct move2 move) {
    int j, from, to;

    from = move.m[0] % 256;
    to = move.m[1] % 256;
    from = from - (from / 9);
    to = to - (to / 9);
    from -= 5;
    to -= 5;
    j = from % 4;
    from -= j;
    j = 3 - j;
    from += j;
    j = to % 4;
    to -= j;
    j = 3 - j;
    to += j;
    from++;
    to++;
}

/**
 * purpose: entry point to checkers. find a move on iboard b for color
 * in the time specified by maxtime, write the best move in
 * iboard, returns information on the search in str
 * returns 1 if a move is found & executed, 0, if there is no legal
 * move in this position.
 */
int checkers(uint8_t color) {
    int numberofmoves;
    int eval;
    struct move2 best, movelist[MAXMOVES];

    /* check if there is only one move */
    numberofmoves = generatecapturelist(movelist, color);
    if(numberofmoves == 1) {
        domove(movelist[0]);
        return(1); /* forced capture */
    } else if (numberofmoves == 0) {
        numberofmoves = generatemovelist(movelist, color);
        if(numberofmoves == 1) {
            domove(movelist[0]);
            return(1); /* only one move */
        }
        if(numberofmoves == 0) {
            return(0); /* no legal moves */
        }
    }
    
    eval = firstalphabeta(1, -10000, 10000, color, &best);
    
    movetonotation(best);
    domove(best);
    
    return eval;
}


/**
 * purpose: search the game tree and find the best move.
 */
int firstalphabeta(int depth, int alpha, int beta, uint8_t color, struct move2 *best) {
    int i;
    int numberofmoves;
    int capture;
    struct move2 movelist[MAXMOVES];

    if (*play) {
        return 0;
    }
    /* test if captures are possible */
    capture = testcapture(color);

    /* recursion termination if no captures and depth=0*/
    if(depth == 0) {
        if(capture == 0) {
            return(evaluation(color));
        } else {
            depth = 1;
        }
    }

    /* generate all possible moves in the position */
    if(capture == 0) {
        numberofmoves = generatemovelist(movelist, color);
        /* if there are no possible moves, we lose: */
        if(numberofmoves == 0)  {
            if (color == BLACK) {
                return(-5000);
            } else {
                return(5000);
            }
        }
    } else {
        numberofmoves = generatecapturelist(movelist, color);
    }

    /* for all moves: execute the move, search tree, undo move. */
    for(i = 0; i < numberofmoves; i++) {
	int value;
        if((os_GetCSC()) == 0x0F) {
            *play = 1;
            return 0;
        }
        
        domove(movelist[i]);

        value = alphabeta(depth - 1, alpha, beta, (color ^ CHANGECOLOR));

        undomove(movelist[i]);
        if(color == BLACK) {
            if(value >= beta) {
                return(value);
            }
            if(value > alpha) {
                alpha = value;
                *best = movelist[i];
            }
        }
        if(color == WHITE) {
            if(value <= alpha) {
                return(value);
            }
            if(value < beta)   {
                beta = value;
                *best = movelist[i];
            }
        }
    }
    if(color == BLACK) {
        return(alpha);
    }
    return(beta);
}

/**
 * purpose: search the game tree and find the best move.
 */
int alphabeta(int depth, int alpha, int beta, uint8_t color) {
    int i;
    int capture;
    int numberofmoves;
    struct move2 movelist[MAXMOVES];

    if (*play) {
        return 0;
    }
    /* test if captures are possible */
    capture = testcapture(color);

    /* recursion termination if no captures and depth=0*/
    if(depth == 0) {
        if(capture == 0) {
            return(evaluation(color));
        } else {
            depth = 1;
        }
    }

    /* generate all possible moves in the position */
    if(capture == 0) {
        numberofmoves = generatemovelist(movelist, color);
        /* if there are no possible moves, we lose: */
        if(numberofmoves == 0)  {
            if (color == BLACK) {
                return(-5000);
            } else {
                return(5000);
            }
        }
    } else {
        numberofmoves = generatecapturelist(movelist, color);
    }
    
    /* for all moves: execute the move, search tree, undo move. */
    for(i = 0; i < numberofmoves; i++) {
        int value;
        domove(movelist[i]);

        value = alphabeta(depth - 1, alpha, beta, color ^ CHANGECOLOR);

        undomove(movelist[i]);

        if(color == BLACK) {
            if(value >= beta) {
                return(value);
            }
            if(value > alpha) {
                alpha = value;
            }
        }
        if(color == WHITE) {
            if(value <= alpha) {
                return(value);
            }
            if(value < beta) {
                beta = value;
            }
        }
    }
    if(color == BLACK) {
        return(alpha);
    }
    return(beta);
}

void domove(struct move2 move) {
    int i;

    for(i = 0; i < move.n; i++) {
        int square = (move.m[i] % 256);
        int after = ((move.m[i] >> 16) % 256);
        cboard[square] = after;
    }
}

void undomove(struct move2 move) {
    int i;

    for(i = 0; i < move.n; i++) {
        int square = (move.m[i] % 256);
        int before = ((move.m[i] >> 8) % 256);
        cboard[square] = before;
    }
}

int evaluation(uint8_t color) {
    uint8_t i;
    int eval;
    int v1, v2;
    int nbm, nbk, nwm, nwk;
    int nbmc = 0, nbkc = 0, nwmc = 0, nwkc = 0;
    int nbme = 0, nbke = 0, nwme = 0, nwke = 0;
    int code = 0;
    static int value[17] = {0, 0, 0, 0, 0, 1, 256, 0, 0, 16, 4096, 0, 0, 0, 0, 0, 0};
    static int edge[14] = {5, 6, 7, 8, 13, 14, 22, 23, 31, 32, 37, 38, 39, 40};
    static int center[8] = {15, 16, 20, 21, 24, 25, 29, 30};
    static int row[41] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 3, 3, 3, 3, 4, 4, 4, 4, 0, 5, 5, 5, 5, 6, 6, 6, 6, 0, 7, 7, 7, 7};
    static int safeedge[4] = {8, 13, 32, 37};

    /* back rank guard */
    static int brg[16] = { 0, -1, 1, 0, 1, 1, 2, 1, 1, 0, 7, 4, 2, 2, 9, 8 };

    int tempo = 0;
    int nm, nk;

    const int turn = 2; //color to move gets +turn
    const int brv = 3;  //multiplier for back rank
    const int kcv = 5;  //multiplier for kings in center
    const int mcv = 1;  //multiplier for men in center

    const int mev = 1;  //multiplier for men on edge
    const int kev = 5;  //multiplier for kings on edge
    const int cramp = 5; //multiplier for cramp

    const int opening = -2; // multipliers for tempo
    const int midgame = -1;
    const int endgame = 2;
    const int intactdoublecorner = 3;


    int backrank;

    for(i = 5; i <= 40; i++) {
        code += value[cboard[i]];
    }

    nwm = code % 16;
    nwk = (code >> 4) % 16;
    nbm = (code >> 8) % 16;
    nbk = (code >> 12) % 16;


    v1 = 100 * nbm + 130 * nbk;
    v2 = 100 * nwm + 130 * nwk;

    eval = v1 - v2;                        /*material values*/
    eval += (250 * (v1 - v2)) / (v1 + v2); /*favor exchanges if in material plus*/

    nm = nbm + nwm;
    nk = nbk + nwk;
    /*--------- fine evaluation below -------------*/

    if(color == BLACK) {
        eval += turn;
    } else {
        eval -= turn;
    }
    /*    (white)
    37  38  39  40
    32  33  34  35
    28  29  30  31
    23  24  25  26
    19  20  21  22
    14  15  16  17
    10  11  12  13
    5   6   7   8
    (black)   */
    
    /* cramp */
    if(cboard[23] == (BLACK | MAN) && cboard[28] == (WHITE | MAN)) {
        eval += cramp;
    }
    if(cboard[22] == (WHITE | MAN) && cboard[17] == (BLACK | MAN)) {
        eval -= cramp;
    }

    /* back rank guard */

    code = 0;
    if(cboard[5] & MAN) {
        code++;
    }
    if(cboard[6] & MAN) {
        code += 2;
    }
    if(cboard[7] & MAN) {
        code += 4;
    }
    if(cboard[8] & MAN) {
        code += 8;
    }
    backrank = brg[code];

    code = 0;
    if(cboard[37] & MAN) {
        code += 8;
    }
    if(cboard[38] & MAN) {
        code += 4;
    }
    if(cboard[39] & MAN) {
        code += 2;
    }
    if(cboard[40] & MAN) {
        code++;
    }
    backrank -= brg[code];
    eval += brv * backrank;

    /* intact double corner */
    if(cboard[8] == (BLACK | MAN)) {
        if(cboard[12] == (BLACK | MAN) || cboard[13] == (BLACK | MAN)) {
            eval += intactdoublecorner;
        }
    }

    if(cboard[37] == (WHITE | MAN)) {
        if(cboard[32] == (WHITE | MAN) || cboard[33] == (WHITE | MAN)) {
            eval -= intactdoublecorner;
        }
    }
    /*    (white)
    37  38  39  40
    32  33  34  35
    28  29  30  31
    23  24  25  26
    19  20  21  22
    14  15  16  17
    10  11  12  13
    5   6   7   8
    (black)   */

    /* center control */
    for(i = 0; i < 8; i++) {
        if(cboard[center[i]] != FREE) {
            if(cboard[center[i]] == (BLACK | MAN)) {
                nbmc++;
            }
            if(cboard[center[i]] == (BLACK | KING)) {
                nbkc++;
            }
            if(cboard[center[i]] == (WHITE | MAN)) {
                nwmc++;
            }
            if(cboard[center[i]] == (WHITE | KING)) {
                nwkc++;
            }
        }
    }
    eval += (nbmc - nwmc) * mcv;
    eval += (nbkc - nwkc) * kcv;

    /*edge*/
    for(i = 0; i < 14; i++) {
        if(cboard[edge[i]] != FREE) {
            if(cboard[edge[i]] == (BLACK | MAN)) {
                nbme++;
            }
            if(cboard[edge[i]] == (BLACK | KING)) {
                nbke++;
            }
            if(cboard[edge[i]] == (WHITE | MAN)) {
                nwme++;
            }
            if(cboard[edge[i]] == (WHITE | KING)) {
                nwke++;
            }
        }
    }
    eval -= (nbme - nwme) * mev;
    eval -= (nbke - nwke) * kev;

    /* tempo */
    for(i = 5; i < 41; i++) {
        if(cboard[i] == (BLACK | MAN)) {
            tempo += row[i];
        }
        if(cboard[i] == (WHITE | MAN)) {
            tempo -= 7 - row[i];
        }
    }

    if(nm >= 16) {
        eval += opening * tempo;
    }
    if((nm <= 15) && (nm >= 12)) {
        eval += midgame * tempo;
    }
    if(nm < 9) {
        eval += endgame * tempo;
    }

    for(i = 0; i < 4; i++) {
        if(nbk + nbm > nwk + nwm && nwk < 3) {
            if(cboard[safeedge[i]] == (WHITE | KING)) {
                eval -= 15;
            }
        }
        if(nwk + nwm > nbk + nbm && nbk < 3) {
            if(cboard[safeedge[i]] == (BLACK | KING)) {
                eval += 15;
            }
        }
    }

    /* the move */
    if(nwm + nwk - nbk - nbm == 0) {
	int stonesinsystem = 0;
	int j = 0;
        if(color == BLACK) {
            for(i = 5; i <= 8; i++) {
                for(; j < 4; j++) {
                    if(cboard[i + 9 * j] != FREE) {
                        stonesinsystem++;
                    }
                }
            }
            if(stonesinsystem % 2) {
                if(nm + nk <= 12) {
                    eval++;
                }
                if(nm + nk <= 10) {
                    eval++;
                }
                if(nm + nk <= 8) {
                    eval += 2;
                }
                if(nm + nk <= 6) {
                    eval += 2;
                }
            } else {
                if(nm + nk <= 12) {
                    eval--;
                }
                if(nm + nk <= 10) {
                    eval--;
                }
                if(nm + nk <= 8) {
                    eval -= 2;
                }
                if(nm + nk <= 6) {
                    eval -= 2;
                }
            }
        } else {
            for(i = 10; i <= 13; i++) {
                for(j = 0; j < 4; j++) {
                    if(cboard[i + 9 * j] != FREE) {
                        stonesinsystem++;
                    }
                }
            }
            if((stonesinsystem % 2) == 0) {
                if(nm + nk <= 12) {
                    eval++;
                }
                if(nm + nk <= 10) {
                    eval++;
                }
                if(nm + nk <= 8) {
                    eval += 2;
                }
                if(nm + nk <= 6) {
                    eval += 2;
                }
            } else {
                if(nm + nk <= 12) {
                    eval--;
                }
                if(nm + nk <= 10) {
                    eval--;
                }
                if(nm + nk <= 8) {
                    eval -= 2;
                }
                if(nm + nk <= 6) {
                    eval -= 2;
                }
            }
        }
    }


    return(eval);
}



/* MOVE GENERATION */

/**
 * purpose:generates all moves. no captures. returns number of moves
 */
int generatemovelist(struct move2 movelist[MAXMOVES], uint8_t color) {
    int n = 0, m;
    int i;

    if(color == BLACK) {
        for(i = 5; i <= 40; i++) {
            if( (cboard[i]&BLACK) != 0 ) {
                if( (cboard[i]&MAN) != 0 ) {
                    if( (cboard[i + 4] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        if(i >= 32) {
                            m = (BLACK | KING);
                        } else {
                            m = (BLACK | MAN);
                        }
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i + 4;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (BLACK | MAN);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                    if( (cboard[i + 5] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        if(i >= 32) {
                            m = (BLACK | KING);
                        } else {
                            m = (BLACK | MAN);
                        }
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i + 5;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (BLACK | MAN);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                }
                if( (cboard[i]&KING) != 0 ) {
                    if( (cboard[i + 4] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        m = (BLACK | KING);
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i + 4;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (BLACK | KING);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                    if( (cboard[i + 5] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        m = (BLACK | KING);
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i + 5;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (BLACK | KING);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                    if( (cboard[i - 4] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        m = (BLACK | KING);
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i - 4;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (BLACK | KING);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                    if( (cboard[i - 5] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        m = (BLACK | KING);
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i - 5;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (BLACK | KING);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                }
            }
        }
    } else { /* color = WHITE */
        for(i = 5; i <= 40; i++) {
            if( (cboard[i]&WHITE) != 0 ) {
                if( (cboard[i]&MAN) != 0 ) {
                    if( (cboard[i - 4] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        if(i <= 13) {
                            m = (WHITE | KING);
                        } else {
                            m = (WHITE | MAN);
                        }
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i - 4;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (WHITE | MAN);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                    if( (cboard[i - 5] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        if(i <= 13) {
                            m = (WHITE | KING);
                        } else {
                            m = (WHITE | MAN);
                        }
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i - 5;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (WHITE | MAN);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                }
                if( (cboard[i]&KING) != 0 ) { /* or else */
                    if( (cboard[i + 4] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        m = (WHITE | KING);
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i + 4;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (WHITE | KING);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                    if( (cboard[i + 5] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        m = (WHITE | KING);
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i + 5;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (WHITE | KING);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                    if( (cboard[i - 4] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        m = (WHITE | KING);
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i - 4;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (WHITE | KING);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                    if( (cboard[i - 5] & FREE) != 0 ) {
                        movelist[n].n = 2;
                        m = (WHITE | KING);
                        m = m << 8;
                        m += FREE;
                        m = m << 8;
                        m += i - 5;
                        movelist[n].m[1] = m;
                        m = FREE;
                        m = m << 8;
                        m += (WHITE | KING);
                        m = m << 8;
                        m += i;
                        movelist[n].m[0] = m;
                        n++;
                    }
                }
            }
        }
    }
    return(n);
}

/**
 * generate all possible captures
 */
int  generatecapturelist(struct move2 movelist[MAXMOVES], uint8_t color) {
    int n = 0;
    int m;
    int i;
    int tmp;

    if(color == BLACK) {
        for(i = 5; i <= 40; i++) {
            if( (cboard[i] & BLACK) != 0) {
                if( (cboard[i] & MAN) != 0) {
                    if( (cboard[i + 4] & WHITE) != 0) {
                        if( (cboard[i + 8] & FREE) != 0) {
                            movelist[n].n = 3;
                            if(i >= 28) {
                                m = (BLACK | KING);
                            } else {
                                m = (BLACK | MAN);
                            }
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i + 8;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (BLACK | MAN);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i + 4];
                            m = m << 8;
                            m += i + 4;
                            movelist[n].m[2] = m;
                            blackmancapture(&n, movelist, i + 8);
                        }
                    }
                    if( (cboard[i + 5] & WHITE) != 0) {
                        if( (cboard[i + 10] & FREE) != 0) {
                            movelist[n].n = 3;
                            if(i >= 28) {
                                m = (BLACK | KING);
                            } else {
                                m = (BLACK | MAN);
                            }
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i + 10;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (BLACK | MAN);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i + 5];
                            m = m << 8;
                            m += i + 5;
                            movelist[n].m[2] = m;
                            blackmancapture(&n, movelist, i + 10);
                        }
                    }
                } else { /* cboard[i] is a KING */
                    if( (cboard[i + 4] & WHITE) != 0) {
                        if( (cboard[i + 8] & FREE) != 0) {
                            movelist[n].n = 3;
                            m = (BLACK | KING);
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i + 8;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (BLACK | KING);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i + 4];
                            m = m << 8;
                            m += i + 4;
                            movelist[n].m[2] = m;
                            tmp = cboard[i + 4];
                            cboard[i + 4] = FREE;
                            cboard[i] = FREE;
                            blackkingcapture(&n, movelist, i + 8);
                            cboard[i + 4] = tmp;
                            cboard[i] = BLACK | KING;
                        }
                    }
                    if( (cboard[i + 5] & WHITE) != 0) {
                        if( (cboard[i + 10] & FREE) != 0) {
                            movelist[n].n = 3;
                            m = (BLACK | KING);
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i + 10;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (BLACK | KING);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i + 5];
                            m = m << 8;
                            m += i + 5;
                            movelist[n].m[2] = m;
                            tmp = cboard[i + 5];
                            cboard[i + 5] = FREE;
                            cboard[i] = FREE;
                            blackkingcapture(&n, movelist, i + 10);
                            cboard[i + 5] = tmp;
                            cboard[i] = BLACK | KING;
                        }
                    }
                    if( (cboard[i - 4] & WHITE) != 0) {
                        if( (cboard[i - 8] & FREE) != 0) {
                            movelist[n].n = 3;
                            m = (BLACK | KING);
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i - 8;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (BLACK | KING);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i - 4];
                            m = m << 8;
                            m += i - 4;
                            movelist[n].m[2] = m;
                            tmp = cboard[i - 4];
                            cboard[i - 4] = FREE;
                            cboard[i] = FREE;
                            blackkingcapture(&n, movelist, i - 8);
                            cboard[i - 4] = tmp;
                            cboard[i] = BLACK | KING;
                        }
                    }
                    if( (cboard[i - 5] & WHITE) != 0) {
                        if( (cboard[i - 10] & FREE) != 0) {
                            movelist[n].n = 3;
                            m = (BLACK | KING);
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i - 10;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (BLACK | KING);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i - 5];
                            m = m << 8;
                            m += i - 5;
                            movelist[n].m[2] = m;
                            tmp = cboard[i - 5];
                            cboard[i - 5] = FREE;
                            cboard[i] = FREE;
                            blackkingcapture(&n, movelist, i - 10);
                            cboard[i - 5] = tmp;
                            cboard[i] = BLACK | KING;
                        }
                    }
                }
            }
        }
    } else { /* color is WHITE */
        for(i = 5; i <= 40; i++) {
            if( (cboard[i] & WHITE) != 0) {
                if( (cboard[i] & MAN) != 0) {
                    if( (cboard[i - 4] & BLACK) != 0) {
                        if( (cboard[i - 8] & FREE) != 0) {
                            movelist[n].n = 3;
                            if(i <= 17) {
                                m = (WHITE | KING);
                            } else {
                                m = (WHITE | MAN);
                            }
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i - 8;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (WHITE | MAN);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i - 4];
                            m = m << 8;
                            m += i - 4;
                            movelist[n].m[2] = m;
                            whitemancapture(&n, movelist, i - 8);
                        }
                    }
                    if( (cboard[i - 5] & BLACK) != 0) {
                        if( (cboard[i - 10] & FREE) != 0) {
                            movelist[n].n = 3;
                            if(i <= 17) {
                                m = (WHITE | KING);
                            } else {
                                m = (WHITE | MAN);
                            }
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i - 10;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (WHITE | MAN);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i - 5];
                            m = m << 8;
                            m += i - 5;
                            movelist[n].m[2] = m;
                            whitemancapture(&n, movelist, i - 10);
                        }
                    }
                } else { /* cboard[i] is a KING */
                    if( (cboard[i + 4] & BLACK) != 0) {
                        if( (cboard[i + 8] & FREE) != 0) {
                            movelist[n].n = 3;
                            m = (WHITE | KING);
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i + 8;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (WHITE | KING);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i + 4];
                            m = m << 8;
                            m += i + 4;
                            movelist[n].m[2] = m;
                            tmp = cboard[i + 4];
                            cboard[i + 4] = FREE;
                            cboard[i] = FREE;
                            whitekingcapture(&n, movelist, i + 8);
                            cboard[i + 4] = tmp;
                            cboard[i] = WHITE | KING;
                        }
                    }
                    if( (cboard[i + 5] & BLACK) != 0) {
                        if( (cboard[i + 10] & FREE) != 0) {
                            movelist[n].n = 3;
                            m = (WHITE | KING);
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i + 10;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (WHITE | KING);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i + 5];
                            m = m << 8;
                            m += i + 5;
                            movelist[n].m[2] = m;
                            tmp = cboard[i + 5];
                            cboard[i + 5] = FREE;
                            cboard[i] = FREE;
                            whitekingcapture(&n, movelist, i + 10);
                            cboard[i + 5] = tmp;
                            cboard[i] = WHITE | KING;
                        }
                    }
                    if( (cboard[i - 4] & BLACK) != 0) {
                        if( (cboard[i - 8] & FREE) != 0) {
                            movelist[n].n = 3;
                            m = (WHITE | KING);
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i - 8;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (WHITE | KING);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i - 4];
                            m = m << 8;
                            m += i - 4;
                            movelist[n].m[2] = m;
                            tmp = cboard[i - 4];
                            cboard[i - 4] = FREE;
                            cboard[i] = FREE;
                            whitekingcapture(&n, movelist, i - 8);
                            cboard[i - 4] = tmp;
                            cboard[i] = WHITE | KING;
                        }
                    }
                    if( (cboard[i - 5] & BLACK) != 0) {
                        if( (cboard[i - 10] & FREE) != 0) {
                            movelist[n].n = 3;
                            m = (WHITE | KING);
                            m = m << 8;
                            m += FREE;
                            m = m << 8;
                            m += i - 10;
                            movelist[n].m[1] = m;
                            m = FREE;
                            m = m << 8;
                            m += (WHITE | KING);
                            m = m << 8;
                            m += i;
                            movelist[n].m[0] = m;
                            m = FREE;
                            m = m << 8;
                            m += cboard[i - 5];
                            m = m << 8;
                            m += i - 5;
                            movelist[n].m[2] = m;
                            tmp = cboard[i - 5];
                            cboard[i - 5] = FREE;
                            cboard[i] = FREE;
                            whitekingcapture(&n, movelist, i - 10);
                            cboard[i - 5] = tmp;
                            cboard[i] = WHITE | KING;
                        }
                    }
                }
            }
        }
    }
    return(n);
}

void blackmancapture(int *n, struct move2 movelist[MAXMOVES], int i) {
    int m;
    int found = 0;
    struct move2 move, orgmove;

    orgmove = movelist[*n];
    move = orgmove;
    
    if( (cboard[i + 4] & WHITE) != 0) {
        if( (cboard[i + 8] & FREE) != 0) {
            move.n++;
            if(i >= 28) {
                m = (BLACK | KING);
            } else {
                m = (BLACK | MAN);
            }
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += (i + 8);
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i + 4];
            m = m << 8;
            m += (i + 4);
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            blackmancapture(n, movelist, i + 8);
        }
    }
    move = orgmove;
    if( (cboard[i + 5] & WHITE) != 0) {
        if( (cboard[i + 10] & FREE) != 0) {
            move.n++;
            if(i >= 28) {
                m = (BLACK | KING);
            } else {
                m = (BLACK | MAN);
            }
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += (i + 10);
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i + 5];
            m = m << 8;
            m += (i + 5);
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            blackmancapture(n, movelist, i + 10);
        }
    }
    if(!found) {
        (*n)++;
    }
}

void  blackkingcapture(int *n, struct move2 movelist[MAXMOVES], int i) {
    int m;
    int tmp;
    int found = 0;
    struct move2 move, orgmove;

    orgmove = movelist[*n];
    move = orgmove;

    if( (cboard[i - 4] & WHITE) != 0) {
        if( (cboard[i - 8] & FREE) != 0) {
            move.n++;
            m = (BLACK | KING);
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += i - 8;
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i - 4];
            m = m << 8;
            m += i - 4;
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            tmp = cboard[i - 4];
            cboard[i - 4] = FREE;
            blackkingcapture(n, movelist, i - 8);
            cboard[i - 4] = tmp;
        }
    }
    move = orgmove;
    if( (cboard[i - 5] & WHITE) != 0) {
        if( (cboard[i - 10] & FREE) != 0) {
            move.n++;
            m = (BLACK | KING);
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += i - 10;
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i - 5];
            m = m << 8;
            m += i - 5;
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            tmp = cboard[i - 5];
            cboard[i - 5] = FREE;
            blackkingcapture(n, movelist, i - 10);
            cboard[i - 5] = tmp;
        }
    }
    move = orgmove;
    if( (cboard[i + 4] & WHITE) != 0) {
        if( (cboard[i + 8] & FREE) != 0) {
            move.n++;
            m = (BLACK | KING);
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += i + 8;
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i + 4];
            m = m << 8;
            m += i + 4;
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            tmp = cboard[i + 4];
            cboard[i + 4] = FREE;
            blackkingcapture(n, movelist, i + 8);
            cboard[i + 4] = tmp;
        }
    }
    move = orgmove;
    if( (cboard[i + 5] & WHITE) != 0) {
        if( (cboard[i + 10] & FREE) != 0) {
            move.n++;
            m = (BLACK | KING);
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += i + 10;
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i + 5];
            m = m << 8;
            m += i + 5;
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            tmp = cboard[i + 5];
            cboard[i + 5] = FREE;
            blackkingcapture(n, movelist, i + 10);
            cboard[i + 5] = tmp;
        }
    }
    if(!found) {
        (*n)++;
    }
}

void  whitemancapture(int *n, struct move2 movelist[MAXMOVES], int i) {
    int m;
    int found = 0;
    struct move2 move, orgmove;

    orgmove = movelist[*n];
    move = orgmove;
    
    if( (cboard[i - 4] & BLACK) != 0) {
        if( (cboard[i - 8] & FREE) != 0) {
            move.n++;
            if(i <= 17) {
                m = (WHITE | KING);
            } else {
                m = (WHITE | MAN);
            }
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += i - 8;
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i - 4];
            m = m << 8;
            m += i - 4;
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            whitemancapture(n, movelist, i - 8);
        }
    }
    move = orgmove;
    if( (cboard[i - 5] & BLACK) != 0) {
        if( (cboard[i - 10] & FREE) != 0) {
            move.n++;
            if(i <= 17) {
                m = (WHITE | KING);
            } else {
                m = (WHITE | MAN);
            }
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += i - 10;
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i - 5];
            m = m << 8;
            m += i - 5;
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            whitemancapture(n, movelist, i - 10);
        }
    }
    if(!found) {
        (*n)++;
    }
}

void whitekingcapture(int *n, struct move2 movelist[MAXMOVES], int i) {
    int m;
    int tmp;
    int found = 0;
    struct move2 move, orgmove;

    orgmove = movelist[*n];
    move = orgmove;

    if( (cboard[i - 4] & BLACK) != 0) {
        if( (cboard[i - 8] & FREE) != 0) {
            move.n++;
            m = (WHITE | KING);
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += i - 8;
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i - 4];
            m = m << 8;
            m += i - 4;
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            tmp = cboard[i - 4];
            cboard[i - 4] = FREE;
            whitekingcapture(n, movelist, i - 8);
            cboard[i - 4] = tmp;
        }
    }
    move = orgmove;
    if( (cboard[i - 5] & BLACK) != 0) {
        if( (cboard[i - 10] & FREE) != 0) {
            move.n++;
            m = (WHITE | KING);
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += i - 10;
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i - 5];
            m = m << 8;
            m += i - 5;
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            tmp = cboard[i - 5];
            cboard[i - 5] = FREE;
            whitekingcapture(n, movelist, i - 10);
            cboard[i - 5] = tmp;
        }
    }
    move = orgmove;
    if( (cboard[i + 4] & BLACK) != 0) {
        if( (cboard[i + 8] & FREE) != 0) {
            move.n++;
            m = (WHITE | KING);
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += i + 8;
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i + 4];
            m = m << 8;
            m += i + 4;
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            tmp = cboard[i + 4];
            cboard[i + 4] = FREE;
            whitekingcapture(n, movelist, i + 8);
            cboard[i + 4] = tmp;
        }
    }
    move = orgmove;
    if( (cboard[i + 5] & BLACK) != 0) {
        if( (cboard[i + 10] & FREE) != 0) {
            move.n++;
            m = (WHITE | KING);
            m = m << 8;
            m += FREE;
            m = m << 8;
            m += i + 10;
            move.m[1] = m;
            m = FREE;
            m = m << 8;
            m += cboard[i + 5];
            m = m << 8;
            m += i + 5;
            move.m[move.n - 1] = m;
            found = 1;
            movelist[*n] = move;
            tmp = cboard[i + 5];
            cboard[i + 5] = FREE;
            whitekingcapture(n, movelist, i + 10);
            cboard[i + 5] = tmp;
        }
    }
    if(!found) {
        (*n)++;
    }
}

/**
 * purpose: test if color has a capture on b
 */
int testcapture(uint8_t color) {
    int i;

    if(color == BLACK) {
        for(i = 5; i <= 40; i++) {
            if( (cboard[i] & BLACK) != 0) {
                if( (cboard[i] & MAN) != 0) {
                    if( (cboard[i + 4] & WHITE) != 0) {
                        if( (cboard[i + 8] & FREE) != 0) {
                            return(1);
                        }
                    }
                    if( (cboard[i + 5] & WHITE) != 0) {
                        if( (cboard[i + 10] & FREE) != 0) {
                            return(1);
                        }
                    }
                } else { /* cboard[i] is a KING */
                    if( (cboard[i + 4] & WHITE) != 0) {
                        if( (cboard[i + 8] & FREE) != 0) {
                            return(1);
                        }
                    }
                    if( (cboard[i + 5] & WHITE) != 0) {
                        if( (cboard[i + 10] & FREE) != 0) {
                            return(1);
                        }
                    }
                    if( (cboard[i - 4] & WHITE) != 0) {
                        if( (cboard[i - 8] & FREE) != 0) {
                            return(1);
                        }
                    }
                    if( (cboard[i - 5] & WHITE) != 0) {
                        if( (cboard[i - 10] & FREE) != 0) {
                            return(1);
                        }
                    }
                }
            }
        }
    } else { /* color is WHITE */
        for(i = 5; i <= 40; i++) {
            if( (cboard[i] & WHITE) != 0) {
                if( (cboard[i] & MAN) != 0) {
                    if( (cboard[i - 4] & BLACK) != 0) {
                        if( (cboard[i - 8] & FREE) != 0) {
                            return(1);
                        }
                    }
                    if( (cboard[i - 5] & BLACK) != 0) {
                        if( (cboard[i - 10] & FREE) != 0) {
                            return(1);
                        }
                    }
                } else { /* cboard[i] is a KING */
                    if( (cboard[i + 4] & BLACK) != 0) {
                        if( (cboard[i + 8] & FREE) != 0) {
                            return(1);
                        }
                    }
                    if( (cboard[i + 5] & BLACK) != 0) {
                        if( (cboard[i + 10] & FREE) != 0) {
                            return(1);
                        }
                    }
                    if( (cboard[i - 4] & BLACK) != 0) {
                        if( (cboard[i - 8] & FREE) != 0) {
                            return(1);
                        }
                    }
                    if( (cboard[i - 5] & BLACK) != 0) {
                        if( (cboard[i - 10] & FREE) != 0) {
                            return(1);
                        }
                    }
                }
            }
        }
    }
    return(0);
}
