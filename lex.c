/* TODO
   - please learn how to *b e n c h m a r k* !!!
   - try perfect hash function?
 */

/* NOTES
   - TBD: 'TkTypeGroup'
   - finish 'type_str'
   - handle hashmaping freeing after lex_err
   - use a global ptr for char instead of 10 billion char variables?
   - probably error handle malloc
   - for 'lex_str' handle *THE SOURCE's* line feeds (not the chars) for multiline strings
   - LOLL do i *even* need malloc for tk 'txt' member? sure i might have to write a function to read escape sequences for strings, not even that bad bruh!!!
   - Some time later, format escape sequences, which *might* require *txt* union member of 'struct Tk' to be changed to a char value since the src_txt escape sequences are unforatted and just chars, bla bla bla
   
   - Include token src_column start and line for better debugging

  - Option that doesn't exit compilation on error
n  - Don't lex integer or number using '-' symbol, treat '-' as an operator (unary & binary) consistently
  - Fix
*/

/*
  Has types: 'struct Tk', 'TkType', 'TkTypeGroup'
  Has defs: 'lex_init' & 'lex_next'
*/
#include "lex.h"
#include "util.h"
/*
  Has types: 'struct HashMap'
  Has defs: 'hashmap_init', 'hashmap_free', 'hashmap_put.." bla blabla
*/
#include "hashmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>

#define TAB_WIDTH_GUESS 8       // Assumption, despite ambiguity 

static long src_line = 1;
static long src_column = 0;
static long src_i = 0;
static long src_len;
static char *src_txt;

static const char* keywords[] = {
        "null", "true", "false", // keywords that are *literals*
        "bool", "char", "int", "num",
        "string", "array", "struct",
        "if", "elif", "else",
        "while", "for",
        "switch", "jmp",
        "fn", "ref"
};

static struct HashMap keywords_hashmap;

// NOTE: Might have to change later if wanting variadic args
#define WARN(msg) printf("WARNING (L%ld C%ld): " msg "\n", src_line, src_column)
#define WARN_FMT(msg, ...) printf("WARNING (L%ld C%ld): " msg "\n", src_line, src_column, __VA_ARGS__)
#define LEX_ERR(msg) (fprintf(stderr, "ERROR (L%ld C%ld): " msg "\n", src_line, src_column), exit(EXIT_FAILURE))
#define LEX_ERR_FMT(msg, ...) (fprintf(stderr, "ERROR (L%ld C%ld): " msg "\n", src_line, src_column, __VA_ARGS__), exit(EXIT_FAILURE))

// NOTE: May remove macros and use a variable to just set both of these once per token lexed
#define INCPOS() (src_i++, src_column++)
#define DECPOS() (src_i--, src_column--)
// left operand is guaranteed evaluation *1st*
#define SET_POS(pos) (src_column += (pos - src_i), src_i = pos)
#define GET_C() src_txt[src_i]
#define NEXT_C() src_txt[src_i + 1]
#define PREV_C() src_txt[src_i - 1]

static inline void handle_tab_char(void)
{
        WARN_FMT("Tab character '\\t' width assumed to be %d spaces despite ambigious "
                 "width and interpration, may lead to inaccurate lexing column numbers",
                 TAB_WIDTH_GUESS);
        src_i++;
        src_column += TAB_WIDTH_GUESS;
}

static inline void handle_newline_char(void)
{
        src_i++;
        src_line++;
        src_column = 0;
}

// IF invalid char for escape sequence, -1 is returned
static inline int esc_seq_from_char(char c)
{
        switch (c) {
        case '0': return '\0';
        case 'n': return '\n';
        case 't': return '\t';
        case '\\': return '\\';
        case '\'': return '\'';
        case '"': return '"';
        default: return -1;
        }
}

// support octal integers?
static void lex_bin_int(struct Tk *p_tk)
{
        p_tk->type_group = G_LITERAL;
        p_tk->type = LIT_INT;
        p_tk->value.int_v = 0;
        char c = GET_C();
        // never write this again please
        for (int i = 0; i < 64; i++, (INCPOS(), c = GET_C())) {
                if (c == '0' || c == '1') {
                        p_tk->value.int_v <<= 1 | (c - '0');
                        continue;
                }

                if (isdigit(c))
                        LEX_ERR_FMT("Expected binary digits after binary"
                                    " integer literal prefix '0b',"
                                    " instead got decimal digit '%c'", c);
                else if (isxdigit(c))
                        LEX_ERR_FMT("Expected binary digits after binary"
                                    " integer literal prefix '0b',"
                                    " instead got hexadecimal digit '%c'", c);
                // non-digit character case
                break;
        }

        printf("%ld %c\n", src_column, c);
        if (c == '0' || c == '1')
                LEX_ERR("Binary integer literal exceeds 64 digits, above 64-bit range");
        if (isdigit(c))
                LEX_ERR_FMT("Binary integer literal exceeds 64 digits,"
                        " above 64-bit range AND got decimal digit '%c'", c);
        else if (isxdigit(c))
                LEX_ERR_FMT("Binary integer literal exceeds 64 digits,"
                            " above 64-bit range AND got hexadecimal"
                            " digit '%c'", c);
}

static void lex_hex_int(struct Tk *p_tk)
{
        p_tk->type_group = G_LITERAL;
        p_tk->type = LIT_INT;
        p_tk->value.int_v = 0;
        char c;
        for (int i = 0; i < 8; i++) {
                switch (c = GET_C()) {
                case 'a': case 'A': p_tk->value.int_v <<= 4 | 10; break;
                case 'b': case 'B': p_tk->value.int_v <<= 4 | 11; break;
                case 'c': case 'C': p_tk->value.int_v <<= 4 | 12; break;
                case 'd': case 'D': p_tk->value.int_v <<= 4 | 13; break;
                case 'e': case 'E': p_tk->value.int_v <<= 4 | 14; break;
                case 'f': case 'F': p_tk->value.int_v <<= 4 | 15; break;
                default:
                        if (!isdigit(c))
                                return;
                        p_tk->value.int_v <<= 4 | (c - '0');
                }
                INCPOS();
        }
        if (isxdigit(GET_C()))
                LEX_ERR("Hexadecimal integer literal exceeds 8 digits, above 64-bit range");
}

/*
  IF 'non-fractional_int' is true (1000% sure there is no *decimal point*,
  resulting from attempting to lex an int that would've *overflowed*)

  CHECK for *suffix* 'f', otherwise error, assuming an attempt at lexing a massive 'int'
*/
static void lex_num(struct Tk *p_tk)
{
        p_tk->type_group = G_LITERAL;
        p_tk->type = LIT_NUM;
        // IF success 'src_txt' is advanced beyond the lexed num
        errno = 0;
        char *num_src_txt_end;
        p_tk->value.fp_v = strtod(src_txt + src_i, &num_src_txt_end);
        bool no_decimal = strchr(src_txt + src_i, '.') == NULL;
        // point to *last digit* instead incase of overflow debugging error messages
        SET_POS((num_src_txt_end - src_txt) - 1);

        if (*num_src_txt_end != 'f') {
                // assume a very large 'int' was attempted to be lexed
                if (no_decimal)
                        LEX_ERR("64-bit integer literal overflow");
        } else
                INCPOS();
        if (errno == ERANGE)
                LEX_ERR("floating-point number overflow");
        INCPOS();
}

// probably need a more robust integer lexer
static void lex_dec_int_or_num(struct Tk *p_tk)
{
        p_tk->type_group = G_LITERAL;
        p_tk->type = LIT_INT;
        long int_src_i_start = src_i;
        char c;
        while (true) {
                c = GET_C();
                if (c == '.' || c == 'f')
                        goto try_lex_num;
                if (!isdigit(c))
                        return;
                if (p_tk->value.int_v > INT64_MAX / 10) { printf("asd\n");
                        goto try_lex_num;}
                // C standard guarantees decimal digits are in order
                p_tk->value.int_v *= 10;
                if (p_tk->value.int_v > INT64_MAX - (c - '0'))
                        goto try_lex_num;
                p_tk->value.int_v += (c - '0');
                INCPOS();
        }

try_lex_num:
        p_tk->value.fp_v = 0;
        // to allow rereading previous digit(s)
        SET_POS(int_src_i_start);
        lex_num(p_tk);
}

static void lex_op_other_or_assign(struct Tk *p_tk, enum TkTypeGroup op_type_group,
                        enum TkType op_type, enum TkType assigner_type)
{
        if (GET_C() == '=') {
                p_tk->type_group = G_OP_ASSIGN;
                p_tk->type = assigner_type;
                INCPOS();
        } else {
                p_tk->type_group = op_type_group;
                p_tk->type = op_type;
        }
}

// temp to remove warnings
// will finish
// test this later
static void lex_keyword_or_identifier(struct Tk *p_tk)
{
        char *txt_start = src_txt + src_i;
        char c = GET_C();
        while (isalpha(c) || isdigit(c) || c == '_') {
                INCPOS();
                c = GET_C();
        }
        size_t len = (size_t) (src_column - p_tk->column);
        int keyword_type_enum = hashmap_get_int(&keywords_hashmap,
                                                txt_start, len);
        // -1 means key *not found*
        if (keyword_type_enum == -1) {
                p_tk->type_group = G_MISC;
                p_tk->type = IDENT;
                p_tk->value.txt = malloc(len + 1);
                if (p_tk->value.txt == NULL)
                        LEX_ERR("Failed memory alloc for identifier");
                p_tk->value.txt[len] = '\0';
                memcpy(p_tk->value.txt, txt_start, len);
        }
        else {
                // p_tk->type_group = G_KEYWORD;
                p_tk->type = (enum TkType) keyword_type_enum;
                // make this better
        }
}


// Only *ascii* chars will be supported, may be changed to *utf-8*, and allow wide chars
static void lex_char(struct Tk *p_tk)
{
        p_tk->type_group = G_LITERAL;
        p_tk->type = LIT_CHAR;

        char c = GET_C();
        if (c == '\'')
                LEX_ERR("Character literal cannot be empty");
        if (c == '\\') {
                INCPOS();
                c = GET_C();
                int esc_char = esc_seq_from_char(c);
                if (esc_char == -1)
                        LEX_ERR("Invalid escape sequence");
                p_tk->value.c = (char) esc_char;
                        
        }
        else if (c == '\t')
                handle_tab_char();
        else if (!isalnum(c) && !isspace(c) && !ispunct(c) && c != '_')
                LEX_ERR("Invalid character literal.");
        INCPOS();
        c = GET_C();
        if (c != '\'')
                LEX_ERR("Expected ''' to end character literal.");
        INCPOS();
}

// so efficient!
static void lex_str(struct Tk *p_tk)
{
        p_tk->type_group = G_LITERAL;
        p_tk->type = LIT_STR;
        long src_i_start = src_i;
        long escape_seq_count = 0;
        char c;
        // starts on the char after the beginning '"'
        while ((c = GET_C()) != '"') {
                if (c == '\0' || c == '\n') {
                        // read at last char of string
                        DECPOS();
                        LEX_ERR("Unclosed string literal.");
                }
                else if (c == '\\') {
                        c = NEXT_C();
                        escape_seq_count++;
                        if (c != '\0' && c != '\n')
                                INCPOS();
                }
                INCPOS();
        }

        // why
        long len = (src_i - src_i_start) - escape_seq_count;
        p_tk->value.txt = malloc((size_t) len + 1);
        if (p_tk->value.txt == NULL)
                LEX_ERR("Failed memory alloc for string literal");
        p_tk->value.txt[len] = '\0';
        SET_POS(src_i_start);

        for (long i = 0; i < len; i++) {
                if ((c = GET_C()) == '\\') {
                        INCPOS();
                        c = GET_C();
                        int esc_char = esc_seq_from_char(c);
                        if (esc_char == -1)
                                LEX_ERR("Invalid escape sequence");
                        c = (char) esc_char;
                        INCPOS();
                }
                else if (c == '\t')
                        handle_tab_char();
                // prob needs rework idk for other special chars
                else {
                        if (!isalnum(c) && !isspace(c) && !ispunct(c))
                                LEX_ERR("Invalid character in string literal");
                        INCPOS();
                }
                p_tk->value.txt[i] = c;
        }
        // char after ending '"'
        INCPOS();
}

// whitespace characters are non-printable characters that define text layouts
static bool handle_whitespace(void)
{
        char c = GET_C();
        if (!isspace(c)) return false;
        while (isspace(c = GET_C()))
                if (c == '\t')
                        handle_tab_char();
                else if (c == '\n')
                        handle_newline_char();
                else
                        INCPOS();
        return true;
}

static bool handle_line_comment(void)
{
        char c = GET_C();
        if (c != '/' || NEXT_C() != '/') return false;
        INCPOS();
        for (c = GET_C(); c != '\n' && c != '\0'; c = GET_C())
                if (c == '\t')
                        handle_tab_char();
                else
                        INCPOS();
        return true;
}

static bool handle_multiline_comment(void)
{
        char c = GET_C();
        if (c != '/' || NEXT_C() != '*') return false;
        long comment_src_line = src_line;
        long comment_src_column = src_column;
        INCPOS();
        for (c = GET_C(); c != '*' || NEXT_C() != '/'; c = GET_C())
                if (c == '\0')
                        LEX_ERR_FMT("Unclosed multiline comment "
                                    "starting at (L%ld C%ld)",
                                    comment_src_line,comment_src_column);
                else if (c == '\t')
                        handle_tab_char();
                else if (c == '\n')
                        handle_newline_char();
                else
                        INCPOS();

        // char after ending '*/'
        INCPOS();
        INCPOS();
        return true;
}

// probably the worst thing written on here
static inline void handle_non_lexable(void)
{
        while (handle_whitespace() || handle_line_comment() ||
               handle_multiline_comment());
}

// TBD stuff
static inline void init_keywords_map(void)
{
        hashmap_init(&keywords_hashmap,
                     HASHMAP_INIT_SIZE + HASHMAP_INIT_SIZE / 4);
        for (int i = 0; i < (int) (sizeof keywords / sizeof *keywords); i++)
                // Keyword enums are in order startiing from 'KW_NULL'
                hashmap_put_int(&keywords_hashmap, keywords[i], strlen(keywords[i]),
                                (int) KW_NULL + i);
        
}

static bool REACHED_END = false; // prob a better way to do this

// TODO: deal with 'src_i' & 'column'
// IDK: find a way to clean up repititve code
enum TkType lex_next(struct Tk *p_tk)
{
        // find a better way to handle this lol
        if (REACHED_END == true)
                ERREXIT("Attempt to lex when EOF is reached");
        handle_non_lexable();
        memset(p_tk, 0, sizeof(struct Tk));
        p_tk->line = src_line;
        p_tk->column = src_column;
        
        char c;
        switch (c = GET_C()) {
        case '=':
                INCPOS();
                if (GET_C() == '=') {
                        INCPOS();
                        p_tk->type_group = G_OP_LOGICAL;
                        p_tk->type = OP_EQ;
                }
                else {
                        p_tk->type_group = G_OP_ASSIGN;
                        p_tk->type = OP_AS;
                }
                break;
        case '+':
                INCPOS();
                if (GET_C() == '+') {
                        INCPOS();
                        p_tk->type_group = G_OP_ARITH;
                        p_tk->type = OP_INC;
                } else
                        lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_ADD, OP_ADD_AS);
                break;
        case '-':
                INCPOS();
                if (GET_C() == '-') {
                        INCPOS();
                        p_tk->type_group = G_OP_ARITH;
                        p_tk->type = OP_DEC;
                }
                else
                        lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_SUB, OP_SUB_AS);
                break;
        case '/':
                INCPOS();
                lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_DIV, OP_DIV_AS);
                break;
        case '*':
                INCPOS();
                lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_MUL, OP_MUL_AS);
                break;
        case '!':
                INCPOS();
                if (GET_C() == '=') {
                        INCPOS();
                        p_tk->type_group = G_OP_LOGICAL;
                        p_tk->type = OP_NOT_EQ;
                } else {
                        p_tk->type_group = G_OP_LOGICAL;
                        p_tk->type = OP_NOT;
                } 
                break;
        case '^':
                INCPOS();
                if (GET_C() == '^') {
                        INCPOS();
                        lex_op_other_or_assign(p_tk, G_OP_BITWISE, OP_BXOR, OP_BXOR_AS);
                } else
                        lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_POW, OP_POW_AS);
                break;
        case '%':
                INCPOS();
                lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_MOD, OP_MOD_AS);
                break;
        case '&':
                INCPOS();
                // maybe add incpos before switch statement?
                if (GET_C() == '&') {
                        INCPOS();
                        p_tk->type_group = G_OP_LOGICAL;
                        p_tk->type = OP_AND;
                } else
                        lex_op_other_or_assign(p_tk, G_OP_BITWISE, OP_BAND, OP_BAND);
                break;
        case '|':
                INCPOS();
                if (GET_C() == '|') {
                        INCPOS();
                        p_tk->type_group = G_OP_LOGICAL;
                        p_tk->type = OP_OR;
                } else
                        lex_op_other_or_assign(p_tk, G_OP_BITWISE, OP_BOR, OP_BOR_AS);
                break;
        case '<':
                INCPOS();
                if (GET_C() == '<') {
                        INCPOS();
                        lex_op_other_or_assign(p_tk, G_OP_BITWISE, OP_BSHL, OP_BSHL_AS);
                } else {
                        if (GET_C() == '=') {
                                INCPOS();
                                p_tk->type_group = G_OP_LOGICAL;
                                p_tk->type = OP_LESS_OR_EQ;
                        } else { 
                                p_tk->type_group = G_OP_LOGICAL;
                                p_tk->type = OP_LESS;
                        }
                }
                break;
        case '>':
                INCPOS();
                if (GET_C() == '>') {
                        INCPOS();
                        lex_op_other_or_assign(p_tk, G_OP_BITWISE, OP_BSHR, OP_BSHR_AS);
                } else {
                        if (GET_C() == '=') {
                                INCPOS();
                                p_tk->type_group = G_OP_LOGICAL;
                                p_tk->type = OP_GREATER_OR_EQ;
                        } else { 
                                p_tk->type_group = G_OP_LOGICAL;
                                p_tk->type = OP_GREATER;
                        }
                }
                break;
        case '[':
                INCPOS();
                p_tk->type_group = G_MISC;
                p_tk->type = BRACKET_L;
                break;
        case ']':
                INCPOS();
                p_tk->type_group = G_MISC;
                p_tk->type = BRACKET_R;
                break;
        case '{':
                INCPOS();
                p_tk->type_group = G_MISC;
                p_tk->type = BRACE_L;
                break;
        case '}':
                INCPOS();
                p_tk->type_group = G_MISC;
                p_tk->type = BRACE_R;
                break;
        case '(':
                INCPOS();
                p_tk->type_group = G_MISC;
                p_tk->type = PAREN_L;
                break;
        case ')':
                INCPOS();
                p_tk->type_group = G_MISC;
                p_tk->type = PAREN_R;
                break;
        case '?':
                INCPOS();
                p_tk->type_group = G_MISC;
                p_tk->type = QUESTION;
                break;
        case ':':
                INCPOS();
                p_tk->type_group = G_MISC;
                p_tk->type = COLON;
                break;
        case '\'':        // 'LIT_CHAR'
                INCPOS();
                lex_char(p_tk);
                break;
        case '"':        // 'LIT_STR'
                INCPOS();
                lex_str(p_tk);
                break;
        case '0':
                INCPOS();
                c = GET_C();
                if (c == 'x') {
                        INCPOS();
                        lex_hex_int(p_tk);
                } else if (c == 'b') {
                        INCPOS();
                        lex_bin_int(p_tk);
                } else {
                        DECPOS();
                        lex_dec_int_or_num(p_tk);
                }
                break;
        case '.':
                // no incpos, 'lex_num' needs to read '.' for fraction
                if (isdigit(NEXT_C()))
                        lex_num(p_tk);
                else {
                        INCPOS();
                        p_tk->type_group = G_MISC;
                        p_tk->type = PERIOD;
                }
                break;
        case '_':
                lex_keyword_or_identifier(p_tk);
                break;
        case '\0':
                if (src_i != src_len)
                        LEX_ERR("Null terminator '\\0' should be at end of file");
                p_tk->type_group = G_MISC;
                p_tk->type = END;
                // cleanup, since end of file
                hashmap_free(&keywords_hashmap);
                REACHED_END = true;
                break;
        default:
                if (isdigit(c))
                        lex_dec_int_or_num(p_tk);
                else if (isalpha(c))
                        lex_keyword_or_identifier(p_tk);
                else
                        // Should have more debugging, to handle non-printable characters
                        // Default argument promotions promote 'c' to 'int', but explicitness just to be fine
                        if (isprint(c))
                                LEX_ERR_FMT("Invalid symbol '%c', character code of %d", c, (int) c);
                        else
                                LEX_ERR_FMT("Invalid non-printable symbol, character code of %d", (int) c);
        }

        return p_tk->type;
}

// should probably rework this to buffer instead of copying into a file
void lex_init(const char *file_name)
{
        FILE *src_file;
        if ((src_file = fopen(file_name, "r")) == NULL)
                PERREXIT("Failed to open source file");

        // fseek & ftell for portability to windows too ig???
        if (fseek(src_file, 0, SEEK_END) != 0)
                goto read_err;
        if ((src_len = ftell(src_file)) == -1L)
                goto read_err;
        if (fseek(src_file, 0, SEEK_SET) != 0)  // heard setting it to start is safe, i'm paranoid tho
                goto read_err;
        // I would use a VLA but I can't gracefully handle those errors if a stack overflow happens
        if ((src_txt = malloc((size_t) (src_len + 1))) == NULL)
                goto read_err;
        fread(src_txt, 1, (size_t) src_len, src_file);
        if (ferror(src_file))
                goto read_err;

        goto read_success;

read_err:
        free(src_txt);
        perror("Failed to read source file");
        if (fclose(src_file) != 0)
                PERREXIT("Failed to close source file");
        
read_success:
        if (fclose(src_file) != 0) {
                free(src_txt);
                PERREXIT("Failed to close source file");
        }
        src_txt[src_len] = '\0';
        // debug
        printf("Source file size: %ld\n", src_len);
        // debug end

        init_keywords_map();
}
