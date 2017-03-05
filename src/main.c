/**
 * @file	Checkers Game
 * @version 1.0
 *
 * By Matthew Waltz
 */
 
/* Keep these headers */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

/* Standard headers */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>

/* Shared libraries */
#include <lib/ce/graphx.h>
#include <lib/ce/fileioc.h>

#include "simplech.h"

/* version info */
#define VERSION       2

#define GRAY_COLOR    0x4A

#define HOME_LOC      131
#define BLOCK_SIZE    26
#define NUM_BLOCKS    8

#define BLACK_COLOR   0x0A
#define WHITE_COLOR   0x0B
#define SEP_COLOR     0x0C
#define BPIECE_COLOR  gfx_black
#define WPIECE_COLOR  gfx_white
#define BACK_COLOR    gfx_white

#define USER_INPUT    0
#define AI_INPUT      1

/* globals */
/* board[0][0] is bottom left corner */
/* board[7][7] is top right corner */
uint8_t board[8][8];

/* Put function prototypes here */
void print_settings(void);
void get_settings(void);
void init_board(void);
void draw_board(void);
void print_home(void);
void game_loop(void);
void draw_logo(void);
void run_game(void);
bool game_over(void);
void game_reset(void);
void draw_box(uint8_t c, uint8_t col, uint8_t row);
uint8_t get_player_color(void);
bool can_jump(int8_t r1, int8_t c1, int8_t r2, int8_t c2, int8_t r3, int8_t c3);
int check_jump(int8_t row, int8_t col);
bool check_move(void);
bool check_board_jumps(void);
void draw_controls(void);
void load_save(void);
void save_save(void);
void draw_red_text(char *text, uint16_t x, uint8_t y);

const char *me = "matt \"mateoconlechuga\" waltz";
const char *them = "engine by martin fierz";
const char *new_game = "new game";
const char *load_game = "load game";
const char *settings_str = "game setup";
const char *mode_str = "mode";
const char *play_as_str = "play as";
const char *will_start_str = "will start";
const char *no_str = "no";
const char *yes_str = "yes";
const char *human1_str = "human v calc";    // mode 0
const char *calc2_str = "calc v calc";      // mode 1
const char *human2_str = "human v human";   // mode 2
const char *black_str = "black";
const char *white_str = "white";
const char *arrows_str = "use the <> arrows to set";
const char *info_str = "these settings will only affect a new game";
const char *white_turn_str = "white's turn";
const char *black_turn_str = "black's turn";
const char *appvar_name = "checkers";

typedef struct player_struct {
	int8_t row;
	int8_t col;
	int8_t selrow;
	int8_t selcol;
	int8_t testrow;
	int8_t testcol;
	bool draw_selection;
	bool jumping;
	uint8_t input;
} player_t;
	
typedef struct settings_struct {
	int8_t mode;
	uint8_t start;
	uint8_t playingas;
} settings_t;

/* some fast access globals */
uint8_t home_item;
uint8_t settings_item;
unsigned steps;
settings_t settings;
player_t player[2];
uint8_t current_player;
uint8_t play_as;

/* Put all your code here */
void main(void) {
	ti_var_t savefile;
	gfx_Begin( gfx_8bpp );
	
	/* enter the main game loop */
	game_loop();

	/* archive the save file */
	if( (savefile = ti_Open(appvar_name,"r")) ) {
		ti_SetArchiveStatus(true,savefile);
	}
	ti_CloseAll();
	
	/* close the graphics and return to the OS */
	gfx_End();
	prgm_CleanUp();
}

/**
 * initializes the board to the default values
 */
void init_board(void) {
	uint8_t y, x, i = 0;
	memset(board, 0, sizeof(board));

	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			i++;
			if(i & 1) {
				if (y < 3) {
					board[x][y] = BLACK | MAN;
				} else if (y > 4) {
					board[x][y] = WHITE | MAN;
				} else {
					board[x][y] = FREE;
				}
			}
		}
		i++;
	}
}

/**
 * Draws the board in the buffer
 */
void draw_board(void) {
	uint16_t x;
	uint8_t y, i = 0;
	uint8_t tmp;

	/* clear the buffer before we try to draw anything */
	gfx_FillScreen(BACK_COLOR);
	
	/* draw all the lines */
	gfx_SetColor(SEP_COLOR);
	for(y = 14; y <= BLOCK_SIZE * NUM_BLOCKS + 14; y += BLOCK_SIZE) {
		gfx_HorizLine_NoClip(14, y, BLOCK_SIZE * NUM_BLOCKS);
	}
	for(x = 14; x <= BLOCK_SIZE * NUM_BLOCKS + 14; x += BLOCK_SIZE) {
		gfx_VertLine_NoClip(x, 14, BLOCK_SIZE * NUM_BLOCKS + 1);
	}

	/* draw the rectangles */
	for(x = 14; x < BLOCK_SIZE * NUM_BLOCKS + 14; x += BLOCK_SIZE) {
		for(y = 14; y < BLOCK_SIZE * NUM_BLOCKS + 14; y += BLOCK_SIZE) {
			i++;
			gfx_SetColor((i & 1) ? WHITE_COLOR : BLACK_COLOR);
			gfx_FillRectangle_NoClip(x + 2, y + 2, BLOCK_SIZE - 3, BLOCK_SIZE - 3);
		}
		i++;
	}

	/* set the index color to the separation lines */
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			tmp = board[x][y];
			if(tmp & ~FREE) {
				gfx_SetColor( (tmp & WHITE) ? BPIECE_COLOR : WPIECE_COLOR);
				gfx_FillCircle(x * BLOCK_SIZE + 14 + 13, (7 - y)*BLOCK_SIZE + 14 + 13, 9);
				if (tmp & KING) {
					gfx_SetColor( (tmp & WHITE) ? WPIECE_COLOR : BPIECE_COLOR);
					gfx_Circle(x * BLOCK_SIZE + 14 + 13, (7 - y)*BLOCK_SIZE + 14 + 13, 3);
				}
			}
		}
	}
}

void game_loop(void) {
	uint8_t key = 1;
	home_item = 1;
	
	for(;;) {
		boot_ClearVRAM();
		gfx_SetDrawBuffer();
		gfx_SetTextFGColor( GRAY_COLOR );
		gfx_SetTextBGColor( BACK_COLOR );
	
		do {
			/* wait until 2nd or enter or clear is pressed before continuing */
			while(key != 0x09 && key != 0x36 && key != 0x0F) {
				if(key) {
					print_home();
					gfx_SwapDraw();
				}
				key = os_GetCSC();

				/* down pressed */
				if (key == 0x01) {
					home_item = (home_item + 1) % 3;
				}
				/* up pressed */
				if (key == 0x04) {
					home_item = home_item - 1 < 0 ? 2 : home_item - 1;
				}
			}

			if (key == 0x0F) {
				break;
			}

			if (home_item == 2) {
				get_settings();
			}
			key = 1;
		} while(home_item == 2);

		if (key == 0x0F) {
			break;
		}

		/* clear the stuffs */
		gfx_FillScreen(BACK_COLOR);
		gfx_SwapDraw();

		/* set the palette colors */
		gfx_palette[BLACK_COLOR] = gfx_RGBTo1555(103, 59, 32);
		gfx_palette[WHITE_COLOR] = gfx_RGBTo1555(234, 208, 151);
		gfx_palette[SEP_COLOR] = gfx_RGBTo1555(143, 106, 64);

		/* reset the board */
		init_board();

		/* check if we need to load the save file */
		if (home_item == 1) {
			load_save();
		} else {
			game_reset();
		}
		draw_board();
		gfx_SwapDraw();
		run_game();
	}
	boot_ClearVRAM();
}

/**
 * reset the game
 */
void game_reset(void) {
	steps = 0;
	current_player = settings.start;
	memset(&player[0], 0, sizeof(player_t));
	memset(&player[1], 0, sizeof(player_t));
	player[1].row = player[1].col = 7;
}

/**
 * returns the equivalent storage
 */
uint8_t get_player_color(void) {
	return current_player == 0 ? BLACK : WHITE;
}

/**
 * draws the logo
 */
void draw_logo(void) {
    static const char *name = "Checkers CE";
    
	gfx_FillScreen(BACK_COLOR);
    gfx_SetTextScale(3,3);
    gfx_SetTextFGColor(BLACK_COLOR);
    gfx_PrintStringXY(name, (320 - 235) / 2, (240 - 40) / 2 - 30);
    gfx_SetTextFGColor(GRAY_COLOR);
    gfx_PrintStringXY(name, (320 - 235) / 2 + 2, (240 - 40) / 2 - 30 - 2);
    gfx_SetTextScale(1,1);
}

/**
 * gets all the settings
 */
void get_settings(void) {
	uint8_t key = 1;

	/* wait until 2nd or enter or clear is pressed before continuing */
	while(key != 0x09 && key != 0x36 && key != 0x0F) {
		if(key) {
			print_settings();
		}
		key = os_GetCSC();

		/* down pressed */
		if (key == 0x01) {
			settings_item = (settings_item + 1) % 3;
			if (settings.mode) {
				settings_item = 2;
			}
		}
		/* up pressed */
		if (key == 0x04) {
			settings_item = settings_item - 1 < 0 ? 2 : settings_item - 1;
			if (settings.mode) {
				settings_item = 0;
			}
		}
		/* left pressed */
		if (key == 0x02) {
			if(settings_item == 0) {
				settings.mode = settings.mode - 1 < 0 ? 2 : settings.mode - 1;
			}
		}
		/* right pressed */
		if (key == 0x03) {
			if(settings_item == 0) {
				settings.mode = (settings.mode + 1) & 3;
			}
		}
		if (key == 0x03 || key == 0x02) {
			switch(settings_item) {
			case 1:
				settings.playingas ^= 1;
				break;
			case 2:
				settings.start ^= 1;
				break;
			default:
				break;
			}
		}
	}
}

/**
 * returns true if the move was valid
 * also executes the move
 */
bool check_move() {
	uint8_t val;
	uint8_t oldy = player[current_player].selrow;
	uint8_t oldx = player[current_player].selcol;
	uint8_t y = player[current_player].row;
	uint8_t x = player[current_player].col;
	uint8_t cur = board[x][y];
	uint8_t old = board[oldx][oldy];
	int8_t diffx, diffy;
	bool force_jump = false;

	if ((x == oldx && y == oldy)) {
		return false;
	}
	if (!(cur & FREE)) {
		return false;
	}

    if (player[current_player].jumping == true) {
        force_jump = true;
    } else if (check_board_jumps()) {
        if (!check_jump(oldx, oldy)) {
            return false;
        }
        force_jump = true;
    }
	
	/* compare the values */
	diffx = x - oldx;
	if (abs(diffx) == 1 && !force_jump) {
		diffy = y - oldy;
		if (abs(diffy) != 1) {
			return false;
		}
		if (!(old & KING) && diffy != (play_as == WHITE ? -1 : 1)) {
			return false;
		}
		board[x][y] = board[oldx][oldy];
		board[oldx][oldy] = FREE;
		goto ret_true;
	}
	if (abs(diffx) == 2) {
		diffy = y - oldy;
		if (abs(diffy) != 2) {
			return false;
		}
		if (!(old & KING) && diffy != (play_as == WHITE ? -2 : 2)) {
			return false;
		}
		val = board[x - diffx / 2][y - diffy / 2];
		if (((val & WHITE) && (play_as == BLACK)) || ((val & BLACK) && (play_as == WHITE))) {
			board[x][y] = board[oldx][oldy];
			board[oldx][oldy] = FREE;
			board[x - diffx / 2][y - diffy / 2] = FREE;
			/* need to continue jumping */
            player[current_player].jumping = (bool)check_jump(x, y);
			goto ret_true;
		}
		return false;
	}
	return false;
ret_true:
	if ((y == 7 && play_as == BLACK) || (y == 0 && play_as == WHITE)) {
		board[x][y] |= KING;
		board[x][y] &= ~MAN;
	}
	return true;
}

/**
 * prints the settings text
 */
void print_settings(void) {
	char *str = NULL;
	draw_logo();
	gfx_SetTextXY(85, (240 - 8) / 2 + 10);
	gfx_PrintString(settings_item == 0 ? "\x10 " : "");
	gfx_PrintString(mode_str);
	switch(settings.mode) {
	case 0:
		str = human1_str;
		break;
	case 1:
		str = calc2_str;
		break;
	case 2:
		str = human2_str;
		break;
	default:
		break;
	}

	gfx_PrintStringXY(str, 180, (240 - 8) / 2 + 10);

	gfx_SetTextXY(85, (240 - 8) / 2 + 22);
	gfx_PrintString(settings_item == 1 ? "\x10 " : "");
	gfx_PrintString(play_as_str);
	gfx_PrintStringXY(settings.playingas ? white_str : black_str, 180, (240 - 8) / 2 + 22);

	if (settings.mode) {
		gfx_SetColor( BACK_COLOR );
		gfx_HorizLine_NoClip(85, (240 - 8) / 2 + 26, 320 - 95);
		gfx_SetColor( GRAY_COLOR );
	}

	gfx_SetTextXY(85, (240 - 8) / 2 + 34);
	gfx_PrintString(settings_item == 2 ? "\x10 " : "");
	gfx_PrintString(will_start_str);
	gfx_PrintStringXY(settings.start ? white_str : black_str, 180, (240 - 8) / 2 + 34);
	
	gfx_PrintStringXY(arrows_str, (320 - gfx_GetStringWidth(arrows_str)) / 2, 240 - 16);
	gfx_PrintStringXY(info_str, (320 - gfx_GetStringWidth(info_str)) / 2, 240 - 16 - 16);
	gfx_HorizLine_NoClip(0, 200, 320);
	gfx_SwapDraw();
}

/**
 * draws a rectangle outline at the col,row posisition
 */
void draw_box(uint8_t c, uint8_t col, uint8_t row) {
	gfx_SetColor(c);
	gfx_Rectangle(15 + BLOCK_SIZE * col, 15 + BLOCK_SIZE * (7 - row), BLOCK_SIZE - 1, BLOCK_SIZE - 1);
}

/**
 * easy way to highlight red text
 */
void draw_red_text(char *text, uint16_t x, uint8_t y) {
	gfx_SetTextFGColor( gfx_red );
	gfx_PrintStringXY( text, x, y );
	gfx_SetTextFGColor( GRAY_COLOR );
}

/**
 * runs the actual game
 */
void run_game(void) {
	uint8_t row, col;
	uint8_t key = 1;
	int exit_key = 0;
	play_as = get_player_color();

	player[0].jumping = false;
	player[1].jumping = false;

	switch(settings.mode) {
	case 0:
		player[settings.playingas].input = USER_INPUT;
		player[settings.playingas ^ 1].input = AI_INPUT;
		break;
	case 1:
		player[0].input = AI_INPUT;
		player[1].input = AI_INPUT;
		break;
	case 2:
		player[0].input = USER_INPUT;
		player[1].input = USER_INPUT;
		break;
	default:
		break;
	}

	gfx_SetTextFGColor( GRAY_COLOR );
	gfx_SetDrawScreen();
	draw_controls();
	
	/* wait until 2nd or enter is pressed before continuing */
	while(key != 0x0F) {
		if (player[current_player].input == AI_INPUT) {
			draw_red_text("thinking...", 239, (240 - 8) / 2);
			getmove(board, play_as, &exit_key);
			steps++;
			if (exit_key) {
				break;
			}
			gfx_SetDrawBuffer();
			draw_board();
			current_player ^= 1;
			play_as = get_player_color();
			draw_controls();
			gfx_SwapDraw();
			gfx_SetDrawScreen();
			if(game_over()) {
				break;
			}
			key = 1;
		}
		if (key) {
			if (player[current_player].input == USER_INPUT) {
				if (check_board_jumps()) {
					draw_red_text("jump", 254, 32);
				}
				draw_box(0xE0, player[current_player].col, player[current_player].row);
				if (player[current_player].draw_selection) {
					draw_box(0xE5, player[current_player].selcol, player[current_player].selrow);
				}
			}
		}
		key = os_GetCSC();
		if (key) {
			draw_box(BACK_COLOR, player[current_player].col, player[current_player].row);
		}

		/* up pressed */
		if (key == 0x04) {
			player[current_player].row = (player[current_player].row + 1) % 8;
		}
		/* down pressed */
		if (key == 0x01) {
			player[current_player].row = player[current_player].row - 1 < 0 ? 7 : player[current_player].row - 1;
		}
		/* right pressed */
		if (key == 0x03) {
			player[current_player].col = (player[current_player].col + 1) % 8;
		}
		/* left pressed */
		if (key == 0x02) {
			player[current_player].col = player[current_player].col - 1 < 0 ? 7 : player[current_player].col - 1;
		}
		if (key == 0x30) {
			if (player[current_player].jumping == false) {
				draw_box(BACK_COLOR, player[current_player].selcol, player[current_player].selrow);
				player[current_player].draw_selection = false;
			}
		}
		if (key == sk_Enter || key == sk_2nd) {
			if (player[current_player].input == USER_INPUT) {
				col = player[current_player].col;
				row = player[current_player].row;

				if (player[current_player].draw_selection) {
					if(check_move()) {
						player[current_player].draw_selection = false;
						draw_box(BACK_COLOR, player[current_player].selcol, player[current_player].selrow);
						gfx_SetDrawBuffer();
						draw_board();
						if (player[current_player].jumping == false) {
							current_player ^= 1;
						}
						steps++;
						play_as = get_player_color();
						draw_controls();
						gfx_SwapDraw();
						gfx_SetDrawScreen();
						if(game_over()) {
							break;
						}
					}
				}
				if (board[col][row] & get_player_color()) {
					draw_box(BACK_COLOR, player[current_player].selcol, player[current_player].selrow);
					player[current_player].selcol = col;
					player[current_player].selrow = row;
					player[current_player].draw_selection = true;
				}
			}
		}
	}
	/* save everything */
	save_save();
	gfx_SetDrawBuffer();
}

/**
 * prints the homescreen text
 */
void print_home(void) {
	draw_logo();
	gfx_SetTextXY(HOME_LOC, (240 - 8) / 2 + 10);
	gfx_PrintString(home_item == 0 ? "\x10 " : "");
	gfx_PrintString(new_game);

	gfx_SetTextXY(HOME_LOC, (240 - 8) / 2 + 22);
	gfx_PrintString(home_item == 1 ? "\x10 " : "");
	gfx_PrintString(load_game);

	gfx_SetTextXY(HOME_LOC, (240 - 8) / 2 + 34);
	gfx_PrintString(home_item == 2 ? "\x10 " : "");
	gfx_PrintString(settings_str);

	gfx_SetColor( GRAY_COLOR );
	gfx_HorizLine_NoClip(0, 200, 320);
	gfx_PrintStringXY(them, (350 - gfx_GetStringWidth(them)) / 2, 240 - 16);
	gfx_PrintStringXY(me, (350 - gfx_GetStringWidth(me)) / 2, 240 - 16 - 16);
}

/**
 * draws the stuffs
 */
void draw_controls(void) {
	gfx_PrintStringXY(current_player ? black_turn_str : white_turn_str, 230, 15);
	gfx_PrintStringXY("steps - ", 230, 49);
	gfx_PrintUInt(steps, 4);
}

/**
 * checks to see if the board has any jumps available
 */
bool check_board_jumps(void) {
	bool ret = false;
	bool tmp = false;
	int8_t r, c;
	for(r = 0; r < 8; r++) {
		for(c = 0; c < 8; c++) {
			tmp = check_jump(c, r);
			if(tmp && player[current_player].jumping == false) {
				draw_box(0x33, c, r);
			}
			ret |= tmp;
		}
	}
	return ret;
}

int check_jump(int8_t col, int8_t row) {
	if (board[col][row] & play_as) {
		if (can_jump(col, row, col + 1, row + 1, col + 2, row + 2)) {
			return 1;
		}
		if (can_jump(col, row, col - 1, row + 1, col - 2, row + 2)) {
			return 2;
		}
		if (can_jump(col, row, col + 1, row - 1, col + 2, row - 2)) {
			return 4;
		}
		if (can_jump(col, row, col - 1, row - 1, col - 2, row - 2)) {
			return 8;
		}
	}
	return 0;
}

bool can_jump(int8_t c1, int8_t r1, int8_t c2, int8_t r2, int8_t c3, int8_t r3) {
	uint8_t piece = board[c1][r1];
	uint8_t jumping = board[c2][r2];

	if (r3 < 0 || r3 > 7 || c3 < 0 || c3 > 7) {
		return false;
	}

	if (board[c3][r3] != FREE) {
		return false;
	}
	if (jumping & FREE) {
		return false;
	}
	if (piece & FREE) {
		return false;
	}

	if (play_as == WHITE) {
		if ((jumping & WHITE)) {
			return false;
		}
		if (((piece & MAN) && r3 > r1)) {
			return false;
		}
	} else {
		if ((jumping & BLACK)) {
			return false;
		}
		if (((piece & MAN) && r3 < r1)) {
			return false;
		}
	}
	return true;
}

/**
 * loads the save file
 */
void load_save(void) {
	ti_var_t save;

	ti_CloseAll();

	save = ti_Open(appvar_name, "r+");

	if(save) {
		if (ti_GetC(save) == VERSION) {
			if (ti_Read(board, sizeof(board), 1, save) != 1) {
				return;
			}
			if (ti_Read(&settings, sizeof(settings_t), 1, save) != 1) {
				return;
			}
			if (ti_Read(&player[0], sizeof(player_t), 1, save) != 1) {
				return;
			}
			if (ti_Read(&player[1], sizeof(player_t), 1, save) != 1) {
				return;
			}
			if (ti_Read(&steps, sizeof(unsigned), 1, save) != 1) {
				return;
			}
			if (ti_Read(&current_player, sizeof(unsigned), 1, save) != 1) {
				return;
			}
		} else {
			return;
		}
	}

	ti_CloseAll();
}

/**
 * saves the save file
 */
void save_save(void) {
	ti_var_t save;

	ti_CloseAll();

	save = ti_Open(appvar_name, "w");

	if(save) {
		ti_PutC(VERSION, save);
		if (ti_Write(board, sizeof(board), 1, save) != 1) {
			goto err;
		}
		if (ti_Write(&settings, sizeof(settings_t), 1, save) != 1) {
			goto err;
		}
		if (ti_Write(&player[0], sizeof(player_t), 1, save) != 1) {
			goto err;
		}
		if (ti_Write(&player[1], sizeof(player_t), 1, save) != 1) {
			goto err;
		}
		if (ti_Write(&steps, sizeof(unsigned), 1, save) != 1) {
			goto err;
		}
		if (ti_Write(&current_player, sizeof(unsigned), 1, save) != 1) {
			goto err;
		}
	}

	ti_CloseAll();

	return;
	
err:
	ti_Delete(appvar_name);
}

bool game_over(void) {
	uint8_t ret = 0;
	uint8_t r, c;
	for(r = 0; r < 8; r++) {
		for(c = 0; c < 8; c++) {
			ret |= board[c][r] & play_as;
		}
	}
	if (!ret) {
	uint8_t key;
		draw_red_text(play_as == BLACK ? "black wins!" : "white wins!", 237, (240 - 8) / 2);
		do {
			key = os_GetCSC();
		} while(key != 0x0F && key != 0x36 && key != 0x09);
		init_board();

		/* reset the game */
		game_reset();
	}
	return ret == 0 ? 1 : 0;
}