#include <stdint.h>

enum TkType {
        // assignment operators
        OP_AS,
        OP_ADD_AS,
        OP_SUB_AS,
        OP_MUL_AS,
        OP_DIV_AS,
        OP_POW_AS,
        OP_MOD_AS,
        OP_BSHL_AS,
        OP_BSHR_AS, 
        OP_BNOT_AS,
        OP_BOR_AS,
        OP_BAND_AS,
        OP_BXOR_AS,

        // arithmetic ops
        OP_ADD,
        OP_SUB,
        OP_INC,
        OP_DEC,
        OP_MUL,
        OP_DIV,
        OP_POW,
        OP_MOD,

        // logical ops
        OP_EQ,
        OP_NOT,
        OP_NOT_EQ,
        OP_LESS,
        OP_GREATER,
        OP_LESS_OR_EQ,
        OP_GREATER_OR_EQ,
        OP_AND,
        OP_OR,

        // bitwise ops
        OP_BSHL,        // shift left
        OP_BSHR,        // shift right
        OP_BAND,
        OP_BNOT,
        OP_BXOR,
        OP_BOR,

        // keywords
        KW_NULL, // bad
        KW_TRUE, // bad 2
        KW_FALSE,
        KW_BOOL,
        KW_CHAR,
        KW_INT,
        KW_NUM,
        KW_STRING,
        KW_ARRAY,
        KW_STRUCT,
        //        KW_DYNAMIC,
        //        KW_CONST,
        KW_IF,
        KW_ELIF,
        KW_ELSE,
        KW_WHILE,
        KW_FOR,
        KW_SWITCH,
        KW_JMP,
        //        KW_DO_WHILE, // remove?
        KW_FN,
        KW_REF,

        // literals
        LIT_CHAR,
        LIT_INT,
        LIT_NUM,
        LIT_STR,
        LIT_NIL,

        // misc
        /*
          misc tokens can be interpreted in *various* ways,
          or are only ones serving *unique* purposes
        */
        IDENT,
        END = 5000, // aka 'eof' but conflicts with C macros
        PAREN_L,
        PAREN_R,
        BRACKET_L,
        BRACKET_R,
        BRACE_L,
        BRACE_R,
        PERIOD,
        COLON,
        SEMICOLON,
        QUESTION,
};

enum TkTypeGroup
{
        G_OP_ASSIGN,
        G_OP_ARITH,
        G_OP_LOGICAL,
        G_OP_BITWISE,
        G_TYPE_SPEC,
        G_STMT,
        
        //        G_DECL, declare
        G_LITERAL,
        G_MISC
};

struct Tk {
        // Add 'type_group' type_str for debugging?
        union {
                char *txt;  // Used by 'LIT_STR' & 'IDENTIFIER'
                int64_t int_v;         // Used by 'LIT_INT'
                double fp_v;           // Used by 'LIT_FP'
                char c;                // Used by 'LIT_CHAR'
        } value;
        const char *type_str;
        long line;      // ftell is archaic and returns a 'long', thus 'len' *also* has to be a 'long'
        long column;
        enum TkTypeGroup type_group;
        enum TkType type;
};

void lex_init(const char* file_name);
enum TkType lex_next(struct Tk *p_tk); // Returns '1' if token type is *not* 'END'
