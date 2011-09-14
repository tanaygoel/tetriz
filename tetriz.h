#ifndef __TETRIZ_H__
#define __TETRIZ_H__

#define GAME_VERSION    "0.9"

#define WINDOW_MAIN_SIZE_X	80
#define WINDOW_MAIN_SIZE_Y	24

#define DIFFICULTY_LEVEL_MIN	1
#define DIFFICULTY_LEVEL_MAX	25

#define KEY_ESCAPE		27

#define GAME_BOARD_HEIGHT		20
#define GAME_BOARD_WIDTH		12

#define ARRAY_LEN(arr)	((int)(sizeof (arr) / sizeof (*(arr))))

enum { SUCCESS, FAILURE };

typedef enum {
    BLOCK_SQUARE,
    BLOCK_LINE,
    BLOCK_TEE,
    BLOCK_ZEE_1,
    BLOCK_ZEE_2,
    BLOCK_ELL_1,
    BLOCK_ELL_2,

    TOTAL_BLOCKS,
} block_t;

typedef enum {
    DEG_0,
    DEG_90,
    DEG_180,
    DEG_270,
    TOTAL_DEGREES
} degree_t;

struct point {
    int x;
    int y;
};

struct position {
    struct point pos[4];
};

struct block {
    block_t type;
    struct point origin;
    degree_t orientation;
    int color;
    const struct position *position;
};

struct game_options {
    int increase_difficulty;
    int display_colors;
    int initial_level;
    int clear_on_new_level;
};

struct game_score {
    int level;
    int rows_cleared;
    int total_rows;
    int score;
    int current_highscore;
};

typedef enum {
    INPUT_TIMEOUT,
    INPUT_INVALID,
    INPUT_MOVE_LEFT,
    INPUT_MOVE_RIGHT,
    INPUT_MOVE_DOWN,
    INPUT_MOVE_UP_DROP,
    INPUT_ROTATE_LEFT,
    INPUT_ROTATE_RIGHT,
    INPUT_PAUSE_QUIT,
} input_t;

extern const char *prog_name;	/* the name the program was invoked with */
extern int board[GAME_BOARD_HEIGHT + 2][GAME_BOARD_WIDTH + 2];
extern const struct position positions[TOTAL_BLOCKS][TOTAL_DEGREES];

int snooze(int ms);
int start_new_game(void);
void display_set_options(void);
int initialize_game_elements(void);
input_t fetch_user_input(void);

int initialize_graphics(void);
void deinitialize_graphics(void);
int display_main_menu(void);
void initialize_game_screen(void);
void display_instructions(void);
void display_about_page(void);
int display_quit_dialog(void);

int draw_start_game(void);
void draw_gameover(int reason);
void draw_next_block(block_t type, degree_t orientation);
void draw_game_board(struct block *current);
void draw_score_board(struct game_score *score);
void draw_level_info(int level);
void draw_game_paused(void);
void draw_highscore(int score);
void draw_cleared_rows_animation_1(int *cleared_rows, int count);
void draw_cleared_rows_animation_2(int *cleared_rows, int count);
void draw_cleared_rows_animation_3(int *cleared_rows, int count);

#endif	/* __TETRIZ_H__ */

/* vim: set ai ts=4 sw=4 tw=80 expandtab: */
