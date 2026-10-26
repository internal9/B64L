// TBD: open & handle all files here and just pass 'FILE *'s to VM, lex, & bytecode_gen
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

       // src file name with flags '-c', '-r', & '-rp', otherwise bytecode file name with flag '-rb'
        const char *file_name = argv[2];
        // assume flag '-r'
        if (argc == 2) {
                goto default_flag;
        }
        else if (argc == 3) {
                const char *flag = argv[1];
                if (strcmp(flag, COMPILE_SRC_ONLY)) {
                        const char *bytecode_file_name = argv[3];
                        lex_init(file_name);
                        //bytecode_gen
                        // bytes = bytecode_gen_file();
                }
                else if (strcmp(flag, RUN_SRC_NO_BYTECODE_FILE)) {
                default_flag:
                        lex_init(file_name);
                        //vm_run(bytecode_gen_nofile(file_name));
                }
                else if (strcmp(flag, RUN_SRC_OUTPUT_BYTECODE_FILE)) {
                        // const char *bytecode_file_name = argv[3];
                }
                else if (strcmp(flag, RUN_BYTECODE_FILE)) {

                }
                else
                        ERREXIT("Invalid flag '%s'\n", flag);
        }

        return EXIT_SUCCESS;
}
