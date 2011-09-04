#include "tetriz.h"

#include <string.h>
#include <stdlib.h>
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

static int do_initialization(int argc, char **argv)
{
    int ret = argc; /* FIXME: assignment is to avoid warning for unused argc */

    /* get the name the program was invoked with */
    prog_name = strrchr(*argv, '/');
    prog_name = prog_name ? (prog_name + 1) : *argv;

    /* install exit-handler */
    if (atexit(deinitialize_graphics)) {
        fprintf(stderr, "%s: failed to install exit-handlers\n", prog_name);
        return FAILURE;
    }

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

