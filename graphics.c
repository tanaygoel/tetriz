#include "tetriz.h"
#include <ncurses.h>

#define PRINT_BLOCK(window, pos_y, pos_x)                           \
    do {                                                            \
        mvwaddch((window), (pos_y), ((pos_x) << 1), ACS_CKBOARD);   \
        waddch((window), ACS_CKBOARD);                              \
    } while (0)

#define GAME_INPUT_TIMEOUT      1000    /* getch() timeout during gameplay */

static WINDOW *win_main = NULL;
static WINDOW *win_game = NULL;
static WINDOW *win_quit = NULL;
static WINDOW *win_next = NULL;
static WINDOW *win_score = NULL;
static WINDOW *win_menu = NULL;
static WINDOW *win_options = NULL;

//static void fill_window(WINDOW *win);

int snooze(int ms)
{
    return napms(ms);
}

int display_main_menu(void)
{
    int i;
    int choice = 0;
    int input = 0;
    int need_refresh = 1;
    static int first_time = 1;

    static const char *logo[] = {
    "  ##########  #########  ##########  #######     ##########  ###########",
    " ##########  #########  ##########  #########   ##########  ##########",
    "    ###     ###            ###     ###    ###      ###          ####",
    "   ###     ######         ###     ########        ###         ####",
    "  ###     ###            ###     ###  ###        ###        ####",
    " ###     #########      ###     ###    ###   ##########   ##########",
    "###     #########      ###     ###     ###  ##########  ###########",
    "                                                                   v " 
        GAME_VERSION    /* concatenated with the last string */
    };

    static const char *options[] = {
        " 1.      N E W   G A M E     ", 
        " 2.  I N S T R U C T I O N S ", 
        " 3.       O P T I O N S      ", 
        " 4.         A B O U T        ", 
        " 0.          E X I T         ",
    };

    werase(win_main);
    wborder(win_main, 0, 0, 0, 0, 0, 0, 0, 0);
    wrefresh(win_main);

    for (i = 0; i < ARRAY_LEN(logo); i++) {
        if (first_time)
            napms(350);
        mvwaddstr(win_main, i + 2, 5, logo[i]);
        wrefresh(win_main);
    }

    if (first_time)
        napms(850);

    werase(win_menu);
    wborder(win_menu, 0, 0, 0, 0, 0, 0, 0, 0);

    flushinp();

    /* the loop will accept only ENTER or SPACE */
    while (input != '\n' && input != ' ') {

        if (need_refresh) {
            for (i = 0; i < ARRAY_LEN(options); i++) {
                if (i != choice)
                    mvwaddstr(win_menu, 2 + (i << 1), 3, options[i]);
                else {
                    wattron(win_menu, A_REVERSE | A_BOLD);
                    mvwaddstr(win_menu, 2 + (i << 1), 3, options[i]);
                    wattroff(win_menu, A_REVERSE | A_BOLD);
                }
            }

            wrefresh(win_menu);
        }

        need_refresh = 1;
        input = wgetch(win_menu);

        switch (input) {
            case KEY_UP:	case 'w':	case 'W':
                if (choice == 0)
                    choice = ARRAY_LEN(options) - 1;
                else
                    choice--;
                break;
            case KEY_DOWN:	case 's':	case 'S':
                if (choice == ARRAY_LEN(options) - 1)
                    choice = 0;
                else
                    choice++;
                break;
            case '0':
                choice = ARRAY_LEN(options) - 1;
                break;
            default:
                if (input > '0' && input < ('0' + ARRAY_LEN(options)))
                    choice = (input - '0' - 1);
                else
                    need_refresh = 0;
                break;
        }
    }

    first_time = 0;
    return (choice + 1) % ARRAY_LEN(options);
}


/* check if the current window size is big enough */
int evaluate_screensize(void)
{
    int status = SUCCESS;
    int current_x, current_y;

    getmaxyx(stdscr, current_y, current_x);
    if (current_x < WINDOW_MAIN_SIZE_X || current_y < WINDOW_MAIN_SIZE_Y) {
        endwin();
        fprintf(stderr, "error: your terminal size [%2d x %2d] is not "
                "sufficient to play the game. The minimum required size is "
                "[%2d x %2d]\n", current_x, current_y, WINDOW_MAIN_SIZE_X,
                WINDOW_MAIN_SIZE_Y);
        status = FAILURE;
    }

    return status;
}

int initialize_graphics(void)
{
    initscr();

    /* check if the current window size is big enough */
    if (evaluate_screensize() == FAILURE)
        return FAILURE;

    cbreak();
    noecho();
    curs_set(0);

    // FIXME: in case the screen size is larger than the required size, 
    // the game should be placed at the center of the window...
    win_main    = newwin(WINDOW_MAIN_SIZE_Y, WINDOW_MAIN_SIZE_X, 0, 0);
    win_game    = newwin(20, 24, 2, 14);
    win_quit    = newwin(7, 24, 7, 14);
    win_next    = newwin(4, 8, 5, 48);
    win_score   = newwin(7, 8, 14, 63);
    win_menu    = newwin(13, 35, 10, 22);
    win_options = newwin(13, 50, 6, 15);

    if (!win_main || !win_game || !win_quit || !win_next || 
            !win_score || !win_menu || !win_options) {
        fprintf(stderr, "%s: failed to initialize windows\n", __func__);
        return 2;
    }

    keypad(stdscr, TRUE);
    keypad(win_menu, TRUE);
    keypad(win_options, TRUE);
    keypad(win_game, TRUE);
    keypad(win_quit, TRUE);

    // FIXME: doesn't work with all ncurses library versions
    set_escdelay(200);

    /* TODO: this function is incomplete */

    return SUCCESS;
}


void deinitialize_graphics(void)
{
    if (win_options)
        delwin(win_options);
    if (win_menu)
        delwin(win_menu);
    if (win_score)
        delwin(win_score);
    if (win_next)
        delwin(win_next);
    if (win_quit)
        delwin(win_quit);
    if (win_game)
        delwin(win_game);
    if (win_main)
        delwin(win_main);

    erase();
    refresh();
    endwin();
}


void display_instructions(void)
{
    int i;
    static int first_time = 1;
    static const char *instructions_1[] = {
        "Well, the game is really simple. You probably have already played",
        "it before. Anyway, you will be having a random combination of four",
        "blocks falling on the floor. The blocks can be moved sideways and",
        "can be rotated clockwise or counter-clockwise. You need to position",
        "the blocks such that they create a horizontal row of blocks in the",
        "floor, without any gap. Such rows will then be cleared, moving the",
        "other blocks downwards. Your aim is to clear as many rows as you can",
        "before you run out of space. That pretty much sums it all. Enjoy...",
    };
    static const char *instructions_2 = "The controls are as follows:";
    static const char *instructions_3[] = {
        "A (or Left Arrow)      :      Move Left",
        "D (or Right Arrow)     :      Move Right",
        "S (or Down Arrow)      :      Move Down",
        "W (or Up Arrow)        :      Drop the block down",
        "J (or Z)               :      Rotate counter-clockwise",
        "K (or X)               :      Rotate clockwise",
        "Q (or P or Esc)        :      To Pause/Quit the game",
    };

    werase(win_main);
    wborder(win_main, 0, 0, 0, 0, 0, 0, 0, 0);

    mvwhline(win_main, 1, 26, ACS_HLINE, 27);
    wattron(win_main, A_BOLD);
    mvwaddstr(win_main, 2, 28, "I N S T R U C T I O N S");
    wattroff(win_main, A_BOLD);
    mvwhline(win_main, 3, 26, ACS_HLINE, 27);

    wrefresh(win_main);
    if (first_time)
        napms(700);

    for (i = 0; i < ARRAY_LEN(instructions_1); i++) {
        mvwaddstr(win_main, i + 5, 6, instructions_1[i]);
        wrefresh(win_main);
        if (first_time)
            napms(200);
    }

    flushinp();
    if (first_time)
        wgetch(win_main);

    mvwaddstr(win_main, 14, 12, instructions_2);
    wrefresh(win_main);

    if (first_time)
        napms(500);

    for (i = 0; i < ARRAY_LEN(instructions_3); i++) {
        mvwaddstr(win_main, i + 16, 12, instructions_3[i]);
        wrefresh(win_main);
        if (first_time)
            napms(200);
    }

    first_time = 0;
    wgetch(win_main);
}


void display_about_page(void)
{
    int i;
    static int first_time = 1;
    static const char *minilogo[] = {
        "####### ##### ####### ####   ##### #####",
        "   #    #        #    #   #    #      #",
        "   #    ####     #    ####     #     #",
        "   #    #        #    #   #    #    #",
        "   #    #####    #    #    # ##### #####",
    };
    static const char *about[] = {
        " An ncurses-based block-game written in C",
        "            Copyright (c) 2011",
        "Author: Tanay Goel (darkstar.tg@gmail.com)",
        "         Licensed under GNU GPLv3",
        "Hosted at: https://github.com/tanay/tetriz",
    };
    static const char *version = "                Version " GAME_VERSION;

    werase(win_main);
    wborder(win_main, 0, 0, 0, 0, 0, 0, 0, 0);

    mvwhline(win_main, 2, 26, ACS_HLINE, 28);
    wattron(win_main, A_BOLD);
    mvwaddstr(win_main, 3, 33, "A B O U T  M E");
    wattroff(win_main, A_BOLD);
    mvwhline(win_main, 4, 26, ACS_HLINE, 28);

    wrefresh(win_main);
    if (first_time)
        napms(700);

    for (i = 0; i < ARRAY_LEN(minilogo); i++) {
        mvwaddstr(win_main, i + 7, 20, minilogo[i]);
    }

    mvwaddstr(win_main, 13, 19, version);

    wrefresh(win_main);
    if (first_time)
        napms(500);

    for (i = 0; i < ARRAY_LEN(about); i++) {
        if (first_time)
            napms(350);
        mvwaddstr(win_main, i + 16, 20, about[i]);
        wrefresh(win_main);
    }

    first_time = 0;

    flushinp();
    wgetch(win_main);
}


void display_set_options(void)
{
    int i;
    int input = 0;
    int choice = 0;
    int result = FAILURE;           /* whether settings are saved or not */
    int need_refresh = 1;
    static int first_time = 1;
    extern struct game_options game_options;

    int settings[] = {
        game_options.initial_level, game_options.increase_difficulty, 
        game_options.clear_on_new_level, game_options.display_colors, 
    };

    static const char *options[] = {
        "Initial Difficulty Level            : ",
        "Increase Difficulty With New Level  : ", 
        "Clear The Board With New Level      : ",
        "Display Colors [not supported yet]  : ",
    };
    static const char *response[] = { " nope ", " yeah " };
    static const char *end_msg[2] = {
        [SUCCESS] = " Settings applied successfully! ",
        [FAILURE] = "     Settings NOT applied!!     ",
    };
    static const char *empty_row = "                                ";

    werase(win_main);
    wborder(win_main, 0, 0, 0, 0, 0, 0, 0, 0);

    mvwhline(win_main, 2, 26, ACS_HLINE, 28);
    wattron(win_main, A_BOLD);
    mvwaddstr(win_main, 3, 34, "O P T I O N S");
    wattroff(win_main, A_BOLD);
    mvwhline(win_main, 4, 26, ACS_HLINE, 28);

    wrefresh(win_main);
    if (first_time)
        napms(500);

    mvwaddstr(win_main, 20, 15, "Use the ARROW KEYS (or W/A/S/D) "
            "to adjust settings");
    mvwaddstr(win_main, 21, 9, "Press ENTER to save the settings and ESC to"
            " exit without saving");

    wrefresh(win_main);
    if (first_time)
        napms(400);

    werase(win_options);
    wborder(win_options, 0, 0, 0, 0, 0, 0, 0, 0);

    while (input != KEY_ESCAPE && input != '\n') {
        if (need_refresh) {
            for (i = 0; i < ARRAY_LEN(options); i++) {
                mvwaddstr(win_options, 3 + (i << 1), 3, options[i]);

                if (i == choice) {
                    wattron(win_options, A_REVERSE | A_BOLD);
                    if (i == 0)
                        wprintw(win_options, "  %02d  ", settings[0]);
                    else
                        waddstr(win_options, response[settings[i]]);
                    wattroff(win_options, A_REVERSE | A_BOLD);
                }
                else {
                    if (i == 0)
                        wprintw(win_options, "  %02d  ", settings[0]);
                    else
                        waddstr(win_options, response[settings[i]]);
                }
            }

            wrefresh(win_options);
        }

        need_refresh = 1;
        flushinp();
        input = wgetch(win_options);

        switch (input) {
            case KEY_DOWN:	case 's':	case 'S':
                choice = (choice + 1) % ARRAY_LEN(options);
                break;
            case KEY_UP:	case 'w':	case 'W':
                if (choice)
                    choice--;
                else
                    choice = ARRAY_LEN(options) - 1;
                break;
            case KEY_LEFT:	case 'a':	case 'A':
                if (choice)
                    settings[choice] ^= 1;
                else {
                    if (--settings[0] < DIFFICULTY_LEVEL_MIN)
                        settings[0] = DIFFICULTY_LEVEL_MIN;
                }
                break;
            case KEY_RIGHT:	case 'd':	case 'D':
                if (choice)
                    settings[choice] ^= 1;
                else {
                    if (++settings[0] > DIFFICULTY_LEVEL_MAX)
                        settings[0] = DIFFICULTY_LEVEL_MAX;
                }
                break;
            default:
                need_refresh = 0;
                break;
        }
    }

    if (input == '\n') {
        result = SUCCESS;
        game_options.initial_level		 = settings[0];
        game_options.increase_difficulty = settings[1];
        game_options.clear_on_new_level	 = settings[2];
        game_options.display_colors		 = settings[3];
        napms(200);		/* to match the delay in scanning 'ESC' by ncurses */
    }

    /* draw a box displaying the end result */
    wattron(win_options, A_REVERSE | A_BOLD);
    mvwhline(win_options, 4, 9, ACS_HLINE, 32);
    mvwhline(win_options, 8, 9, ACS_HLINE, 32);
    mvwvline(win_options, 5, 8, ACS_VLINE, 3);
    mvwvline(win_options, 5, 41, ACS_VLINE, 3);
    mvwaddch(win_options, 4, 8, ACS_ULCORNER);
    mvwaddch(win_options, 4, 41, ACS_URCORNER);
    mvwaddch(win_options, 8, 8, ACS_LLCORNER);
    mvwaddch(win_options, 8, 41, ACS_LRCORNER);
    mvwaddstr(win_options, 5, 9, empty_row);
    mvwaddstr(win_options, 6, 9, end_msg[result]);
    mvwaddstr(win_options, 7, 9, empty_row);
    wattroff(win_options, A_REVERSE | A_BOLD);

    wtimeout(win_options, 1500);    /* set a 2 seconds timeout */
    wrefresh(win_options);
    flushinp();
    wgetch(win_options);
    wtimeout(win_options, -1);      /* restore the timeout */

    first_time = 0;
}


void initialize_game_screen(void)
{
    werase(win_main);
    wborder(win_main, 0, 0, 0, 0, 0, 0, 0, 0);

    mvwaddstr(win_main,  3, 47, "next block");
    mvwaddstr(win_main, 14, 47, "level        :  ");
    mvwaddstr(win_main, 16, 47, "rows cleared :  ");
    mvwaddstr(win_main, 17, 47, "total rows   :  ");
    mvwaddstr(win_main, 18, 47, "score        :  ");
    mvwaddstr(win_main, 20, 47, "high-score   :  ");

    /* drawing the next block window border */
    mvwaddch(win_main, 4, 46, ACS_ULCORNER);
    mvwaddch(win_main, 4, 57, ACS_URCORNER);
    mvwaddch(win_main, 9, 46, ACS_LLCORNER);
    mvwaddch(win_main, 9, 57, ACS_LRCORNER);

    mvwhline(win_main, 4, 47, ACS_HLINE, 10);
    mvwhline(win_main, 9, 47, ACS_HLINE, 10);
    mvwvline(win_main, 5, 46, ACS_VLINE, 4);
    mvwvline(win_main, 5, 57, ACS_VLINE, 4);

    /* drawing the gameplay window border */
    mvwaddch(win_main,  1, 13, ACS_ULCORNER);
    mvwaddch(win_main,  1, 38, ACS_URCORNER);
    mvwaddch(win_main, 22, 13, ACS_LLCORNER);
    mvwaddch(win_main, 22, 38, ACS_LRCORNER);

    mvwhline(win_main,  1, 14, ACS_HLINE, 24);
    mvwhline(win_main, 22, 14, ACS_HLINE, 24);
    mvwvline(win_main,  2, 13, ACS_VLINE, 20);
    mvwvline(win_main,  2, 38, ACS_VLINE, 20);

    mvwvline(win_main, 1, 11, ACS_VLINE, 22);
    mvwvline(win_main, 1, 40, ACS_VLINE, 22);
    mvwaddch(win_main, 0, 11, ACS_TTEE);
    mvwaddch(win_main, 0, 40, ACS_TTEE);
    mvwaddch(win_main, 23, 11, ACS_BTEE);
    mvwaddch(win_main, 23, 40, ACS_BTEE);

    wrefresh(win_main);
}


input_t fetch_user_input(void)
{
    input_t result;
    int input = wgetch(win_game);

    switch (input) {
        case ERR:                               /* timeout */
            result = INPUT_TIMEOUT;
            break;
        case KEY_LEFT:  case 'A':   case 'a':
            result = INPUT_MOVE_LEFT;
            break;
        case KEY_RIGHT: case 'D':   case 'd':
            result = INPUT_MOVE_RIGHT;
            break;
        case KEY_DOWN:  case 'S':   case 's':
            result = INPUT_MOVE_DOWN;
            break;
        case KEY_UP:    case 'W':   case 'w':
            result = INPUT_MOVE_UP_DROP;
            break;
        case 'J':   case 'j':   case 'Z':   case 'z':
            result = INPUT_ROTATE_LEFT;
            break;
        case 'K':   case 'k':   case 'X':   case 'x':
            result = INPUT_ROTATE_RIGHT;
            break;
        case KEY_ESCAPE:    case 'Q':   case 'P':   case 'q':   case 'p':
            result = INPUT_PAUSE_QUIT;
            break;
        default:
            result = INPUT_INVALID;
            break;
    }

    return result;
}

int display_quit_dialog(void)
{
#define MESSAGE_QUIT()                                  \
    do {                                                \
        mvwhline(win_quit, 3, 2, ACS_HLINE, 8);         \
        mvwhline(win_quit, 5, 2, ACS_HLINE, 8);         \
        mvwaddch(win_quit, 3, 1, ACS_ULCORNER);         \
        mvwaddch(win_quit, 4, 1, ACS_VLINE);            \
        mvwaddch(win_quit, 5, 1, ACS_LLCORNER);         \
        mvwaddch(win_quit, 3, 10, ACS_URCORNER);        \
        mvwaddch(win_quit, 4, 10, ACS_VLINE);           \
        mvwaddch(win_quit, 5, 10, ACS_LRCORNER);        \
        mvwprintw(win_quit, 4, 2, "  QUIT  ");          \
    } while (0)

#define MESSAGE_RESUME()                                \
    do {                                                \
        mvwhline(win_quit, 3, 14, ACS_HLINE, 8);        \
        mvwhline(win_quit, 5, 14, ACS_HLINE, 8);        \
        mvwaddch(win_quit, 3, 13, ACS_ULCORNER);        \
        mvwaddch(win_quit, 4, 13, ACS_VLINE);           \
        mvwaddch(win_quit, 5, 13, ACS_LLCORNER);        \
        mvwaddch(win_quit, 3, 22, ACS_URCORNER);        \
        mvwaddch(win_quit, 4, 22, ACS_VLINE);           \
        mvwaddch(win_quit, 5, 22, ACS_LRCORNER);        \
        mvwprintw(win_quit, 4, 14, " RESUME ");         \
    } while (0)

    enum { QUIT, RESUME };

    int input = 0;
    int choice = RESUME;
    int need_refresh = 1;
    const char *message =	"                        "
                            "       P A U S E D      "
                            "                        "
                            "                        "
                            "                        "
                            "                        "
                            "                        ";

    wattron(win_quit, A_REVERSE | A_BOLD);
    mvwaddstr(win_quit, 0, 0, message);
    wattroff(win_quit, A_REVERSE | A_BOLD);

    /* the loop will accept only ENTER or SPACE */
    while (input != '\n' && input != ' ') {
        if (need_refresh) {
            if (choice == RESUME) {
                MESSAGE_RESUME();
                wattron(win_quit, A_REVERSE | A_BOLD);
                MESSAGE_QUIT();
                wattroff(win_quit, A_REVERSE | A_BOLD);
            } else {
                MESSAGE_QUIT();
                wattron(win_quit, A_REVERSE | A_BOLD);
                MESSAGE_RESUME();
                wattroff(win_quit, A_REVERSE | A_BOLD);
            }

            wrefresh(win_quit);
        }

        need_refresh = 1;
        input = wgetch(win_quit);

        if (input == KEY_ESCAPE) {
            choice = RESUME;        /* equivalent to "RESUME" */
            break;
        }

        switch (input) {
            /* invert choice */
            case KEY_LEFT:
            case 'A':   case 'a':
            case 'J':   case 'j':
            case 'Z':   case 'z':
                choice = QUIT;
                break;
            case KEY_RIGHT:
            case 'D':   case 'd':
            case 'K':   case 'k':
            case 'X':   case 'x':
                choice = RESUME;
                break;

            default:
                need_refresh = 0;
                break;
        }
    }

    werase(win_quit);
    return (choice == RESUME) ? SUCCESS : FAILURE;

#undef MESSAGE_RESUME
#undef MESSAGE_QUIT
}

void draw_next_block(block_t type, degree_t orientation)
{
    int i;

    werase(win_next);
    for (i = 0; i < ARRAY_LEN(positions[type][orientation].pos); i++) {
        PRINT_BLOCK(win_next, 0 + positions[type][orientation].pos[i].y, 
                0 + positions[type][orientation].pos[i].x);
    }
    wrefresh(win_next);
}

void draw_game_board(struct block *block)
{
    int i, j;

    werase(win_game);

    for (i = 1; i < GAME_BOARD_HEIGHT + 1; i++) {
        for (j = 1; j < GAME_BOARD_WIDTH + 1; j++) {
            if (board[i][j])
                PRINT_BLOCK(win_game, i-1, j-1);
        }
    }

    if (block) {
        for (i = 0; i < ARRAY_LEN(block->position->pos); i++) {
            PRINT_BLOCK(win_game, block->origin.y + block->position->pos[i].y, 
                    block->origin.x + block->position->pos[i].x);
        }
    }

    wrefresh(win_game);
}


void draw_score_board(struct game_score *score)
{
    mvwprintw(win_score, 0, 0, "%-8d\n%-8d%-8d%-8d\n%-8d", score->level, 
            score->rows_cleared, score->total_rows, score->score, 
            score->current_highscore);
    wrefresh(win_score);
}


int draw_start_game(void)
{
    int input;
    int ret = SUCCESS;
    const char *message =	"                        "
                            "       R E A D Y ?      "
                            "                        ";

    wattron(win_game, A_REVERSE | A_BOLD);
    mvwprintw(win_game, 7, 0, message);
    wattroff(win_game, A_REVERSE | A_BOLD);

    wrefresh(win_game);
    flushinp();
    input = wgetch(win_game);
    if (input == KEY_ESCAPE || input == 'N' || input == 'n')
        ret = FAILURE;

    /* set timeout for gameplay */
    wtimeout(win_game, GAME_INPUT_TIMEOUT);
    return ret;
}

void draw_gameover(int reason)
{
    const char *message[2] = {
        "                        "
        "    G A M E  O V E R    "      /* if the game ended usually */
        "                        ",
        "                        "
        "    T H A N K  Y O U    "      /* if the user quits the game */
        "                        ",
    };

    wattron(win_game, A_REVERSE | A_BOLD);
    mvwprintw(win_game, 7, 0, message[reason]);
    wattroff(win_game, A_REVERSE | A_BOLD);

    wrefresh(win_game);
    wtimeout(win_game, -1);     /* remove the timeout */
    flushinp();
    wgetch(win_game);
}

void draw_highscore(int score)
{
    const char *message =	"                        "
                            "  N E W                 "
                            "     H I G H S C O R E  "
                            "                        "
                            "          %8d      "
                            "                        ";

    wattron(win_game, A_REVERSE | A_BOLD);
    mvwprintw(win_game, 6, 0, message, score);
    wattroff(win_game, A_REVERSE | A_BOLD);

    wrefresh(win_game);
    wtimeout(win_game, 5000);       /* display this message for 5 seconds */
    flushinp();
    wgetch(win_game);
}

void draw_level_info(int level)
{
    const char *message =	"                        "
                            "     L E V E L   %02d     "
                            "                        ";

    wattron(win_game, A_REVERSE | A_BOLD);
    mvwprintw(win_game, 7, 0, message, level);
    wattroff(win_game, A_REVERSE | A_BOLD);

    wrefresh(win_game);
    wtimeout(win_game, 1500);   /* timeout of 1.5 secs for the getch() below */
    flushinp();
    wgetch(win_game);

    /* restore the timeout for gameplay */
    wtimeout(win_game, GAME_INPUT_TIMEOUT);
}

#if 0
void draw_cleared_rows_animation(int *rows, int count)
{
    int j;
    static const char *empty_row = "                        ";

    napms(400);
    for (j = 0; j < count; j++) {
        mvwprintw(win_game, rows[j], 0, empty_row);
    }
    wrefresh(win_game);
    napms(400);

    for (j = 0; j < count; j++) {
        mvwhline(win_game, rows[j], 0, ACS_CKBOARD, 
                (GAME_BOARD_WIDTH << 1));
    }
    wrefresh(win_game);
    napms(400);

    for (j = 0; j < count; j++) {
        mvwprintw(win_game, rows[j], 0, empty_row);
    }
    wrefresh(win_game);
    napms(400);

    for (j = 0; j < count; j++) {
        mvwhline(win_game, rows[j], 0, ACS_CKBOARD, 
                (GAME_BOARD_WIDTH << 1));
    }
    wrefresh(win_game);
    napms(400);

    for (j = 0; j < count; j++) {
        mvwprintw(win_game, rows[j], 0, empty_row);
    }
    wrefresh(win_game);
    napms(500);
}
#else
void draw_cleared_rows_animation(int *rows, int count)
{
#define board_width (GAME_BOARD_WIDTH << 1)
    static int direction = 0;     /* animation from left or right */

    int i, j;
    int end = direction++ & 1;

    napms(300);
    for (i = 0; i < board_width; i++) {
        for (j = end; j < count; j += 2)
            mvwaddch(win_game, rows[j], i, ' ' | A_NORMAL);
        for (j = !end; j < count; j += 2)
            mvwaddch(win_game, rows[j], (board_width - i - 1), ' ' | A_NORMAL);
        wrefresh(win_game);
        napms(30);
    }
#undef board_width
}
#endif

#if 0
static void fill_window(WINDOW *win)
{
    int i, j, x, y;
    getmaxyx(win, y, x);

    werase(win);
    for (i = 0; i < x; i++) {
        move(i, 0);
        for (j = 0; j < y; j++)
            waddch(win, ACS_CKBOARD);
    }
}
#endif

