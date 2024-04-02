#include <argp.h>
#include "../include/yakvm.h"
#include "arguments.h"

/* available arguments */
static struct argp_option options[] = {
    {},
};

/* parse the arguments */
static error_t parse_opt(int key, char *arg,
                         struct argp_state *state) {
    struct arguments *args = state->input;
    long ret = 0;

    switch (key) {
        case ARGP_KEY_ARG:
            args->bin = arg;
            log(LOG_INFO, "parse_opt() sets bin to %s", arg);
            break;

        case ARGP_KEY_NO_ARGS:
            log(LOG_ERR, "no bin specified");
            argp_usage(state);
            break;

        default:
            ret = ARGP_ERR_UNKNOWN;
            break;
    }

    return ret;
}

static struct argp argp = {
    .options = options,
    .parser = parse_opt,
    .doc = "run the given guest",
    .args_doc = "<bin>",
};

/* parse arguments from *argv* into *arguments* */
void emulator_parse_arguments(struct arguments *args, int argc,
                              char **argv)
{
    argp_parse(&argp, argc, argv, 0, 0, args);
    assert(args->bin);
}
