#include "tetriz.h"
#include <pthread.h>

#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define BASE_SCORE_PER_ROW  10		/* score awarded for each row cleared */
#define MAX_ROWS_PER_LEVEL	20		/* rows to clear before next level */
#define INITIAL_TIMEOUT		1000

/* formula for calculating timeout reduction delta with each new level */
#define TIMEOUT_DELTA(level)    ((DIFFICULTY_LEVEL_MAX - (level) + 1) * 3)

typedef enum {
    ACTION_MOVE_UP_DROP,            /* drop the block at the floor */
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_MOVE_DOWN,
    ACTION_ROTATE_LEFT,
    ACTION_ROTATE_RIGHT,
    ACTION_PLACE_NEW,

    TOTAL_MOVEMENTS
} action_t;

struct thread_data {
    struct block *current;
    int game_over;
    int new_highscore;
    pthread_mutex_t lock;
    pthread_t thread_id;
    int block_dropped_ignore_input;
    /* 
     * XXX: the reason behind the block_dropped_ignore_input is as follows:
     *      when a block is dropped, we want to make sure that it can not be 
     *      moved by the user (this can happen if the user hits some key, and 
     *      the main thread catches it before the other thread can freeze the 
     *      block). The solution, this flag, is an ugly hack to achive the same.
     */
};

static int move_block(struct block *block, action_t movement);
static inline struct block *update_current_block(struct block *block);
static void freeze_block(struct block *current);
static int clear_even_rows(void);
static void *worker_thread_fn(void *);
static int test_movement(struct block *block);
static int update_score_level(struct game_score *score, int num_rows, 
        int *timeout);
static void reset_game_board(void);
static void play_game(struct thread_data *data);

int board[GAME_BOARD_HEIGHT + 2][GAME_BOARD_WIDTH + 2];
struct game_options game_options = {
    1,                                  /* increase difficulty = "yeah" */
    0,                                  /* display_colors = "nope" */
    1,                                  /* initial_level */
    0,                                  /* clear_on_new_level = "nope" */
};

static int global_highscore = 0;
static block_t next_block = -1;
static degree_t next_block_orientation = -1;
static struct block current_block;

static const struct point starting_position = { 4, 0 };
const struct position positions[TOTAL_BLOCKS][TOTAL_DEGREES] = {
    /* BLOCK_SQUARE */
    {
        { { { 1, 1 }, { 2, 1 }, { 1, 2 }, { 2, 2 } } },			/* DEG_0 */
        { { { 1, 1 }, { 2, 1 }, { 1, 2 }, { 2, 2 } } },			/* DEG_90 */
        { { { 1, 1 }, { 2, 1 }, { 1, 2 }, { 2, 2 } } },			/* DEG_180 */
        { { { 1, 1 }, { 2, 1 }, { 1, 2 }, { 2, 2 } } },			/* DEG_270 */
    }, 
    /* BLOCK_LINE */
    {
        { { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 } } },			/* DEG_0 */
        { { { 2, 0 }, { 2, 1 }, { 2, 2 }, { 2, 3 } } },			/* DEG_90 */
        { { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 } } },			/* DEG_180 */
        { { { 2, 0 }, { 2, 1 }, { 2, 2 }, { 2, 3 } } },			/* DEG_270 */
    }, 
    /* BLOCK_TEE */
    {
        { { { 2, 0 }, { 1, 1 }, { 2, 1 }, { 3, 1 } } },			/* DEG_0 */
        { { { 2, 0 }, { 2, 1 }, { 2, 2 }, { 3, 1 } } },			/* DEG_90 */
        { { { 2, 2 }, { 1, 1 }, { 2, 1 }, { 3, 1 } } },			/* DEG_180 */
        { { { 2, 0 }, { 2, 1 }, { 2, 2 }, { 1, 1 } } },			/* DEG_270 */
    }, 
    /* BLOCK_ZEE_1 */
    {
        { { { 2, 0 }, { 1, 1 }, { 2, 1 }, { 1, 2 } } },			/* DEG_0 */
        { { { 1, 1 }, { 2, 1 }, { 2, 2 }, { 3, 2 } } },			/* DEG_90 */
        { { { 2, 0 }, { 1, 1 }, { 2, 1 }, { 1, 2 } } },			/* DEG_180 */
        { { { 1, 1 }, { 2, 1 }, { 2, 2 }, { 3, 2 } } },			/* DEG_270 */
    }, 
    /* BLOCK_ZEE_2 */
    {
        { { { 1, 0 }, { 1, 1 }, { 2, 1 }, { 2, 2 } } },			/* DEG_0 */
        { { { 1, 1 }, { 2, 1 }, { 0, 2 }, { 1, 2 } } },			/* DEG_90 */
        { { { 1, 0 }, { 1, 1 }, { 2, 1 }, { 2, 2 } } },			/* DEG_180 */
        { { { 1, 1 }, { 2, 1 }, { 0, 2 }, { 1, 2 } } },			/* DEG_270 */
    }, 
    /* BLOCK_ELL_1 */
    {
        { { { 1, 0 }, { 1, 1 }, { 1, 2 }, { 2, 2 } } },			/* DEG_0 */
        { { { 1, 1 }, { 2, 1 }, { 3, 1 }, { 1, 2 } } },			/* DEG_90 */
        { { { 1, 0 }, { 2, 0 }, { 2, 1 }, { 2, 2 } } },			/* DEG_180 */
        { { { 2, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } } },			/* DEG_270 */
    }, 
    /* BLOCK_ELL_2 */
    {
        { { { 2, 0 }, { 2, 1 }, { 2, 2 }, { 1, 2 } } },			/* DEG_0 */
        { { { 1, 0 }, { 1, 1 }, { 2, 1 }, { 3, 1 } } },			/* DEG_90 */
        { { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 1, 2 } } },			/* DEG_180 */
        { { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 2, 2 } } },			/* DEG_270 */
    }, 
};


static void reset_game_board(void)
{
    int i;

    memset(board, 0, sizeof (board));
    memset(board[0], ~0, sizeof (*board));
    memset(board[GAME_BOARD_HEIGHT + 1], ~0, sizeof (*board));

    for (i = 0; i < GAME_BOARD_HEIGHT + 2; i++)
        board[i][0] = board[i][GAME_BOARD_WIDTH + 1] = ~0;
}


int initialize_game_elements(void)
{
    /* initialize the board */
    reset_game_board();

    /* initialize the current and next block */
    srand((unsigned) time(NULL));
    next_block = (block_t)(rand() % TOTAL_BLOCKS);
    next_block_orientation = (degree_t)(rand() % TOTAL_DEGREES);

    return SUCCESS;
}


static inline struct block *update_current_block(struct block *block)
{
    if (block == NULL)
        block = &current_block;

    block->color = 1;			/* not used currently */
    block->type = next_block;
    block->orientation = next_block_orientation;

    next_block = (block_t)(rand() % TOTAL_BLOCKS);
    next_block_orientation = (degree_t)(rand() % TOTAL_DEGREES);

    return block;
}

#if 0
static int time_pass(void)
{
    while (1) {
        draw_next_block(next_block, next_block_orientation);
        draw_game_board(&current_block);
        update_current_block(&current_block);
    }

    return 0;
}
#endif

int start_new_game(void)
{
    struct thread_data data = { NULL, 0, 0, PTHREAD_MUTEX_INITIALIZER, 0, 0 };

    initialize_game_elements();
    initialize_game_screen();

    /* draw the next block, and the begin game message */
    draw_next_block(next_block, next_block_orientation);
    if (draw_start_game() == FAILURE)
        return SUCCESS;

    draw_level_info(game_options.initial_level);

    /* create the worker thread */
    if (pthread_create(&data.thread_id, NULL, worker_thread_fn, (void *)&data))
        return FAILURE;

    /* the main thread for the core gameplay */
    play_game(&data);
    pthread_join(data.thread_id, NULL);

    if (data.new_highscore)
        draw_highscore(global_highscore);

    return SUCCESS;
}


static void play_game(struct thread_data *data)
{
    int status;
    input_t input;
    int did_user_quit = 0;    /* 0 if game-over, otherwise if user quits */

    while (1) {
        /* take user input */
        input = fetch_user_input();
        if (input == INPUT_INVALID)
            continue;

        pthread_mutex_lock(&data->lock);

        /* check if the game is still going on */
        if (data->game_over) {
            pthread_mutex_unlock(&data->lock);
            break;
        }

        /* in case there's no "current block", don't do anything */
        if (data->block_dropped_ignore_input || 
                (!data->current && input != INPUT_PAUSE_QUIT)) {
            pthread_mutex_unlock(&data->lock);
            continue;
        }

        /* take appropriate action based on user input */
        switch (input) {
            case INPUT_MOVE_LEFT:
                status = move_block(data->current, ACTION_MOVE_LEFT);
                if (status == SUCCESS)
                    draw_game_board(data->current);
                break;
            case INPUT_MOVE_RIGHT:
                status = move_block(data->current, ACTION_MOVE_RIGHT);
                if (status == SUCCESS)
                    draw_game_board(data->current);
                break;
            case INPUT_MOVE_DOWN:
                status = move_block(data->current, ACTION_MOVE_DOWN);
                if (status == SUCCESS)
                    draw_game_board(data->current);
                break;
            case INPUT_MOVE_UP_DROP:
                status = move_block(data->current, ACTION_MOVE_UP_DROP);
                if (status == SUCCESS) {
                    data->block_dropped_ignore_input = 1;
                    draw_game_board(data->current);
                }
                break;
            case INPUT_ROTATE_LEFT:
                status = move_block(data->current, ACTION_ROTATE_LEFT);
                if (status == SUCCESS)
                    draw_game_board(data->current);
                break;
            case INPUT_ROTATE_RIGHT:
                status = move_block(data->current, ACTION_ROTATE_RIGHT);
                if (status == SUCCESS)
                    draw_game_board(data->current);
                break;
            case INPUT_PAUSE_QUIT:
                status = display_quit_dialog();
                draw_game_board(data->current);
                if (status == FAILURE) {    /* if the user chooses to quit */
                    data->game_over = 1;
                    did_user_quit = 1;
                }
                break;
            case INPUT_TIMEOUT:     /* nothing to do */
                break;
            default:
                break;
        }

        pthread_mutex_unlock(&data->lock);
    }

    draw_gameover(did_user_quit);
}


static int move_block(struct block *block, action_t movement)
{
    int result;
    struct block newblock = *block;	/* start with a copy of the given block */

    /* apply the requested operation */
    switch (movement) {
        case ACTION_MOVE_LEFT:
            --newblock.origin.x;
            assert(newblock.origin.x < GAME_BOARD_WIDTH);
            break;
        case ACTION_MOVE_RIGHT:
            ++newblock.origin.x;
            assert(newblock.origin.x < GAME_BOARD_WIDTH);
            break;
        case ACTION_MOVE_DOWN:
            ++newblock.origin.y;
            assert(newblock.origin.y < GAME_BOARD_HEIGHT);
            break;
        case ACTION_MOVE_UP_DROP:
            /* FIXME: not an optimal solution */
            do {
                newblock.origin.y++;
            } while (test_movement(&newblock) == SUCCESS);

            newblock.origin.y--;
            assert(newblock.origin.y < GAME_BOARD_HEIGHT);
            break;
        case ACTION_ROTATE_LEFT:
            if (newblock.orientation == DEG_0)
                newblock.orientation = TOTAL_DEGREES - 1;
            else
                --newblock.orientation;
            newblock.position = &positions[newblock.type][newblock.orientation];
            break;
        case ACTION_ROTATE_RIGHT:
            newblock.orientation = (newblock.orientation + 1) % TOTAL_DEGREES;
            newblock.position = &positions[newblock.type][newblock.orientation];
            break;
        case ACTION_PLACE_NEW:
            newblock.origin = starting_position;
            newblock.position = &positions[newblock.type][newblock.orientation];
            break;			/* no change in position requested */

        default:
            return FAILURE;
    }

    /* check if the new changes can be applied */
    result = test_movement(&newblock);

    if (result == SUCCESS)
        *block = newblock;		/* apply the new change */

    return result;
}

static int test_movement(struct block *block)
{
    int i;
    int status = SUCCESS;

    for (i = 0; i < ARRAY_LEN(block->position->pos); i++) {
        int x = block->origin.x + block->position->pos[i].x;
        int y = block->origin.y + block->position->pos[i].y;

        if (board[y + 1][x + 1]) {
            status = FAILURE;
            break;
        }
    }

    return status;
}

static void *worker_thread_fn(void *arg)
{
    int i;
    int status;
    int timeout = INITIAL_TIMEOUT;
    struct game_score game_score = { game_options.initial_level, 
        0, 0, 0, global_highscore };
    struct thread_data *data = (struct thread_data *)arg;

    /* setup the initial timeout (by reducing all deltas upto initial_level */
    for (i = 1; i <= game_options.initial_level; i++)
        timeout -= TIMEOUT_DELTA(i);

    draw_score_board(&game_score);

    /* main game loop */
    while (1) {
        /* wait till timeout */
        snooze(timeout);

        /* acquire the lock */
        pthread_mutex_lock(&data->lock);

        if (data->game_over) {
            pthread_mutex_unlock(&data->lock);
            break;
        }

        if (!data->current) {
            data->current = update_current_block(NULL);

            status = move_block(data->current, ACTION_PLACE_NEW);
            if (status == FAILURE) {
                data->game_over = 1;
                pthread_mutex_unlock(&data->lock);
                break;
            }

            draw_next_block(next_block, next_block_orientation);
        } else {
            /* try moving the block downwards */
            status = move_block(data->current, ACTION_MOVE_DOWN);
            if (status == FAILURE) {
                int ret, num_rows;

                /* freeze this block in the game board */
                freeze_block(data->current);
                data->current = NULL;	/* reset the current block pointer */
                data->block_dropped_ignore_input = 0; /* reset this now */

                num_rows = clear_even_rows();
                if (num_rows) {
                    ret = update_score_level(&game_score, num_rows, &timeout);
                    draw_score_board(&game_score);

                    /* see if the level has changed */
                    if (ret) {
                        draw_game_board(data->current);
                        draw_level_info(game_score.level);
                    }
                }
            }
        }

        draw_game_board(data->current);
        pthread_mutex_unlock(&data->lock);
    }

    if (game_score.score > global_highscore) {
        global_highscore = game_score.score;
        data->new_highscore = 1;
    }

    return arg;
}


static int update_score_level(struct game_score *score, int num_rows, 
        int *timeout)
{
    int has_level_changed = 0;

    //score->score += (score->level + SCORE_CLEAR_ROW) * num_rows * num_rows;
    score->score += score->level * num_rows * num_rows * BASE_SCORE_PER_ROW;
    if (score->score > score->current_highscore)
        score->current_highscore = score->score;

    score->total_rows += num_rows;
    score->rows_cleared += num_rows;

    if (score->rows_cleared >= MAX_ROWS_PER_LEVEL) {
        if (game_options.increase_difficulty &&
                score->level < DIFFICULTY_LEVEL_MAX)
            *timeout -= TIMEOUT_DELTA(score->level);

        if (game_options.clear_on_new_level)
            reset_game_board();

        score->level++;
        score->rows_cleared = 0;
        has_level_changed = 1;
    }

    return has_level_changed;
}


/* the nearest empty row (counting from bottom) in the board */
static int top_row = GAME_BOARD_HEIGHT;

static void freeze_block(struct block *current)
{
    int i;
    static const int empty_row[GAME_BOARD_WIDTH + 2] = {
        ~0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,     /* GAME_BOARD_WIDTH times */
        ~0,
    };

    /* fuse the current block with the board */
    for (i = 0; i < ARRAY_LEN(current->position->pos); i++) {
        int x = current->origin.x + current->position->pos[i].x;
        int y = current->origin.y + current->position->pos[i].y;

        assert(board[y + 1][x + 1] == 0);
        board[y + 1][x + 1] = current->color;
    }

    /* now recompute the top_row */
    for (i = GAME_BOARD_HEIGHT; i > 0; i--) {
        if (memcmp((void *)board[i], (void *)empty_row, sizeof (*board)) == 0) {
            top_row = i;
            break;
        }
    }

    assert(top_row > 0);
}


static int clear_even_rows(void)
{
    int abs_row;                /* absolute row number */
    int count = 0;
    int i = GAME_BOARD_HEIGHT;
    int cleared_rows[4];        /* XXX: max 4 rows can be cleared at a time? */

    /* use animation style 3 */
    static void (*clear_animation)(int *, int) = draw_cleared_rows_animation_3;

    for (abs_row = GAME_BOARD_HEIGHT; i > top_row; abs_row--) {
        int j;
        int found = 1;

        for (j = 1; j < GAME_BOARD_WIDTH + 1; j++) {
            if (!board[i][j]) {
                found = 0;
                break;
            }
        }

        if (found)
            cleared_rows[count++] = abs_row - 1;  /* save the absolute row */
        else {
            i--;
            continue;		                /* move on to check the next row */
        }

        memset(board[i], 0, sizeof (*board));   /* clear the current row */
        /* move down the all the rows above the cleared row */
        memmove((void *)board[top_row + 1], (void *)board[top_row], 
                (i - top_row) * sizeof (*board));

        top_row++;
        assert(top_row <= GAME_BOARD_HEIGHT);
    }

    /* now animate (blink) the cleared rows */
    if (count)
        clear_animation(cleared_rows, count);

    return count;
}

/* vim: set ai ts=4 sw=4 tw=80 expandtab: */
