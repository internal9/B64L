/*
  NOTES:
  - '-c' Only compile src and output bytecode file, no execution
  - '-n' Compile & run src, but *no* bytecode file output
  - have a seperate VM binary or just arg option to either run or just bytecode compile?
*/
#include "lex.h"
#include "bytecode_gen.h"
// #include "VM.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define COMPILE_SRC_ONLY             "-c"  // compile src & output bytecode file (w/ specified name) only
#define RUN_SRC_NO_BYTECODE_FILE     "-r"  // run src but bytecode compiled in temp buffer instead of file
#define RUN_SRC_OUTPUT_BYTECODE_FILE "-ro" // run src & output bytecode file (w/ specified name)
#define RUN_BYTECODE_FILE            "-rb" // run bytecode file (NOTE: maybe handle endianness?)

// barebones for testing purposes
int main(int argc, const char *argv[])
{
        if (argc < 2)
                ERREXIT("Expected one source file to compile to bytecode,"
                        " or one bytecode file to execute.");
        if (argc > 3)
                ERREXIT("Excess arguments, expected optional flag and/or"
                        " one source file to compile to bytecode,"
                        " or one bytecode file to execute.");

        const char *file_name = argv[argc - 1];
        // assume 'option -n'
        if (argc == 2) {
                lex_init(file_name);
                bytecode_gen_nofile();
        }
        else if (argc == 3) {
                const char *flag = argv[1];
                if (strcmp(flag, COMPILE_SRC_ONLY)) {
                        lex_init(file_name);
                        // bytes = bytecode_gen_file();
                }
                else if (strcmp(flag, RUN_SRC_NO_BYTECODE_FILE)) {
                        lex_init(file_name);
                        // vm_run(bytecode_gen_nofile());
                }
                else if (strcmp(flag, RUN_SRC_OUTPUT_BYTECODE_FILE) {

                }
                else if (strcmp(flag, RUN_BYTECODE_FILE) {

                }
                else
                        ERREXIT("Invalid flag '%s'\n", flag);
        }

        return EXIT_SUCCESS;
}
