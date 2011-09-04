#include "tetriz.h"

#include <signal.h>
#include <string.h>
#include <stdio.h>

const char *prog_name = NULL;

static int do_initialization(int argc, char **argv);

int main(int argc, char **argv)
{
    int ret;
    int choice;

    ret = do_initialization(argc, argv);		/* initialize everything */
    if (ret)
        return FAILURE;

    do {
        choice = display_main_menu();

        switch (choice) {
            case 1:
                start_new_game();
                break;
            case 2:
                display_instructions();
                break;
            case 3:
                display_set_options();
                break;
            case 4:
                display_about_page();
                break;
            default:
                break;
        }
    } while (choice);

    deinitialize_graphics();

    return 0;
}

static void handle_SIGSEGV(int signal)
{
    deinitialize_graphics();
    raise(signal);
}

static int do_initialization(int argc, char **argv)
{
    int ret = argc;

    /* set SIGSEGV handler */
    /* TODO: implement it using sigaction */
    if (signal(SIGSEGV, handle_SIGSEGV) == SIG_ERR) {
        fprintf(stderr, "%s: failed to install signal handler\n", prog_name);
        return 1;
    }

    /* get the name the program was invoked with */
    prog_name = strrchr(*argv, '/');
    prog_name = prog_name ? (prog_name + 1) : *argv;

    // parse_command_line_arguments(argc, argv);

    ret = initialize_graphics();
    if (ret) {
        fprintf(stderr, "%s: failed to initialize graphics [%d]\n", 
                prog_name, ret);
        return ret;
    }

    ret = initialize_game_elements();
    if (ret) {
        fprintf(stderr, "%s: failed to initialize game elements [%d]\n", 
                prog_name, ret);
        return ret;
    }

    return ret;
}

