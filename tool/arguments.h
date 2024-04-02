#ifndef __YAKVM_TOOL_ARGUMENTS_H_

        #define __YAKVM_TOOL_ARGUMENTS_H_

        struct arguments {
                char *bin; /* path to the guest bin to be used */
        };

        /* parse arguments from *argv* into *args* */
        void emulator_parse_arguments(struct arguments *args,
                                      int argc, char *argv[]);

#endif // __YAKVM_TOOL_ARGUMENTS_H_
