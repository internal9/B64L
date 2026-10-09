// Still need to standard 'lex_next' & 'next_tk' & 'current_tk'

// INT ARRAY name[size1][size2]
// lex literals (e.g. LIT_BOOL)
// init lexing here instead of in main.c?
#include "lex.h"
#include "VM.h"
#include "hashmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// passing pointer prob a lil unnecessary
#define tk_err(p_tk, MSG) (fprintf(stderr, "(L%ld C%ld) " MSG "\n", (p_tk)->line, (p_tk)->column), \
                            exit(EXIT_FAILURE))
#define tk_err_fmt(p_tk, FMT, ...) (fprintf(stderr, "(L%ld C%ld) " MSG "\n", \
                                         (p_tk)->line, (p_tk)->column, __VA_ARGS__), \
                                            exit(EXIT_FAILURE))

// static struct Tk tk;
// TBD: static struct Tk buf[some arbitrary num like 512];

// emit(MOVSI, st_addr(), rm(bsize(arg)), arg)

#define MAX_ARRAY_DIMENSION_COUNT 5

static long ip;

enum DataType {
    BOOL,
    CHAR,
    INT,
    NUM,
    STRING,
    ARRAY,
    STRUCT
};

enum SymType {
    LABEL,
    VAR,
    FUNC,
};

enum {
	SCALAR,
	ARRAY,
	STRUCT
};

union DataTypeInfo {
	union {
	    struct {
	        bool is_size_dynamic;   // if doing this, runtime size checks for safety?
	        enum DataType base_type;
	        int dimension_sizes[MAX_ARRAY_DIMENSION_COUNT]; // max dimension count
	        int dimension_count;
	    } ty_array;
		struct {
			
		} ty_struct_def;
		struct {

		} ty_struct_ref;
	} info;
	enum DataType type;
};

union SymTypeInfo {
	union DataTypeInfo data_type_info;
    struct {
        enum DataType ret_type;
        struct Symbol **params_array;
        int param_count;
    } func;

	struct {

	} label;

    struct {
        struct Symbol **members;
    } struct_type_def;
	
}



// find way to handle scope
// super "efficient"
struct Symbol {
    union DataTypeInfo info;
    /* const char *id; unnecessary? */
    const char *ident;
    enum SymType type;
    long line, column;
    long addr;
    //  const char *enclosing_func_name;
    // bool in_func;
    int block_id;   // one block id for global scope & for each fn?
    int scope_level;
};

// forward reference to a fn, calling a fn that doesn't exist (presumbly declared later / typo / doesn't exist)
struct ForwardFnCall {
    long line, column;
    struct Symbol **params_array;
    int param_count;
    /* default is NULL, *unless* if the fn call
    is in an assignment expression, declaration or not */
    enum DataType var_to_assign_to_type;
};

struct ForwardFnCalls {
    struct ForwardFnCall* calls_array;
    int len;
};

static struct HashMap forward_fn_call_table;

static struct Symbol *gen_decl_var(enum DataType data_type, bool is_func_param);
static bool gen_expr(struct Tk *p_left, int prec_limit, bool is_expr_start);
static void gen_struct();
static void gen_block(void);
static void gen_if();
static void gen_while();
static void gen_for();
static void gen_switch();
static void gen_jmp();

static struct Tk tk;
static struct Tk buf[512];
static int buf_i = 0;
static int tk_count = 0; // prob simplify this

static struct Tk *next_tk(void) {
    if (buf_i == tk_count) {
        lex_next(buf + buf_i);
        tk_count++;
    }
    return buf + buf_i++;
}

static struct Tk *peek_tk(void) {
    if (buf_i == tk_count) {
        lex_next(buf + buf_i);
        tk_count++;
    }
    return buf + buf_i;
}

static struct Tk *current_tk(void) {
    if (buf_i == 0) return NULL; // temp
    return buf + buf_i - 1;
}

/*
// remove?
struct Tk *pop_tk()
{
        return buf + --buf_i;  // leave top as garbage
}
*/

static void tk_buf_clear(void) {
    buf_i = tk_count = 0;
}

static struct HashMap symbol_table;
static int scope_level;
/* static char *enclosing_func_name = NULL; */
static bool in_func = false;
static long addr = 0;
/* static int64_t rsp = -1; ? */

static void
emit_double(double value) {

}

static void
emit_i64(int64_t value) {

}

static void
emit_opcode() {

}


#define INSTR_ARG_REG(opcode) (uint8_t) (0x80 | opcode) // if two args, arg2 is reg, if one arg, arg is reg
#define INSTR_ARG_LIT(opcode) (uint8_t) (0x00 | opcode)

// Temp
#define emit(a)

static struct Symbol *
sym_push(enum SymType type, struct Tk *p_ident_tk) {
    /* pls arena alloc... or just stack allocate */
    struct Symbol *p_sym = malloc(sizeof(struct Symbol));
    p_sym->ident = p_ident_tk->value.txt; // valid as long as tk remains in tk buffer
    p_sym->type = type;
    p_sym->line = p_ident_tk->line;
    p_sym->column = p_ident_tk->column;
    p_sym->scope_level = scope_level;
    p_sym->addr = addr++;
    hashmap_put_ptr(&symbol_table,
                    p_ident_tk->value.txt,
                    strlen(p_ident_tk->value.txt),
                    (void *) p_sym);
    return p_sym;
}

static struct Symbol *
sym_get(struct Tk *p_ident_tk) {
    return hashmap_get_ptr(&symbol_table,
                           p_ident_tk->value.txt,
                           strlen(p_ident_tk->value.txt));
}

static bool
sym_pop(int count) // struct Tk* p_ident_tk) ?
{
    // handle dynamically allocated stuff
    for (int i = 0; i < count; i++) {
        //struct Sym *p_sym = hashmap_pop(&symbol_table);
        //if (p_sym == NULL) return false;
        //free(p_sym);
    }
    return true;
}

/*
void gen_from_ident(struct Tk *p_tk, bool allow_standalone)
{
        switch (lex_next(&tk)) {
        case PAREN_L:
        case BRACKET_L:
        case PERIOD:
        case INC:
        case DEC:
        default:
                if (!allow_standalone) {}
                        ; // error(id cannot be by itself);
        }
}
*/


static inline int op_prec(enum TkType tk_type) {
    switch (tk_type) {
        case OP_ADD:
        case OP_SUB: return 1;
        case OP_MUL:
        case OP_DIV: return 2;
        case OP_POW: return 3;
        default: return -1;
    }
}

// temp debug
static void instr_debug(enum TkType type) {
    switch (type) {
        case OP_ADD: puts("ADD R1, R2");
            break;
        case OP_SUB: puts("SUB R1, R2");
            break;
        case OP_MUL: puts("MUL R1, R2");
            break;
        case OP_DIV: puts("DIV R1, R2");
            break;
        case OP_POW: puts("POW R1, R2");
            break;
        default: break;
    }
}

// static struct Tk buf[512];

// unnamed aggregate objects allowed (e.g. func((int) {'a', 2, 5}); func({.asd = 20})

/*
  Pratt-parser but...
  probably the worst thing written here
*/
/*
  Returns 'true' if it compiled an expression e.g 'a+b',
  rather than a standalone literal e.g. '2'
*/

enum {
    a,
};

static bool
gen_expr(struct Tk *p_left, int prec_limit, bool is_expr_start){
    static bool R1_use = false;

    // temp
    if (p_left->type == END) // TODO: handle unexpected tks too
        puts("Expected expression"), exit(1);

    bool is_left_expr = false;
    switch (p_left->type) {
        case LIT_TRUE: emit(0b10000000 | MOV); emit(1); //emit("MOV R1, 1")
        case LIT_FALSE:
        case LIT_CHAR:
        case LIT_INT: break;
        case LIT_NUM:
        case LIT_STR:
        case PAREN_L:
            is_left_expr = gen_expr(next_tk(), 0, false);
            if (!is_left_expr)
                p_left = current_tk(); // i hate it, since only literal retrieve that
            struct Tk *p_tk = next_tk();
            if (p_tk->type != PAREN_R)
                tk_err(p_tk, "Expected ')'");
            break;
        // andd the rest for later
        default:
            // handle non-expr tk
            break;
    }

    struct Tk *p_op = peek_tk();
    int prec = op_prec(p_op->type);

    // lower prec or no operator
    if (prec <= prec_limit && prec != 3) {
        if (is_left_expr) return true; // compiled an expr with operators
        if (is_expr_start) // as in not being the start of the whole expr
            printf("MOV R1, VAR AT C%ld: %ld\n", // standalone e.g. int a = 1
                   p_left->column, p_left->value.int_v);
        return false; // standalone literal
    }

    do {
        next_tk(); // read past operator tk
        bool is_right_sub_expr = gen_expr(next_tk(), prec, false);
        if (is_right_sub_expr) {
            R1_use = false;
            puts("MOV R2, R1");
        } else {
            struct Tk *p_right = current_tk();
            printf("MOV R2, VAR AT C%ld: %ld\n",
                   p_right->column, p_right->value.int_v);
        }

        if (is_left_expr) {
            if (!R1_use) {
                puts("POP R1");
                R1_use = true;
            }
            // R1 already in use, unpushed
        } else {
            // standalone lit
            if (R1_use)
                puts("PUSH R1");
            else
                R1_use = true;
            printf("MOV R1, VAR AT C%ld: %ld\n", p_left->column, p_left->value.int_v);
        }

        instr_debug(p_op->type);
        if (!is_left_expr) // i don't like this
            is_left_expr = true;

        p_op = peek_tk();
        prec = op_prec(p_op->type);
    } while (prec == 3 || prec > prec_limit);

    // TBD
    if (is_expr_start) {
        // memcpy(&tk, next_tk(), sizeof(struct Tk));
        // tk_buf_clear();
    }
    return true;
}

#define gen_expr_start(p_left_tk) gen_expr(p_left_tk, 0, true)
// account for floating-points later, cuz fp regs

static void
declarator()
{

}

static int
data_type_from_kw(enum TkType kw_type) {
    switch (kw_type) {
        case KW_BOOL: return BOOL;
        case KW_CHAR: return CHAR;
        case KW_INT: return INT;
        case KW_NUM: return NUM;
        case KW_STRING: return STRING;
        case KW_ARRAY: return ARRAY;
        case KW_STRUCT: return STRUCT;
        default: return -1;
    }
}

static void
gen_data(long addr, enum DataType type) {
    // type mismatch checks
    switch (type) {
        case BOOL: // gen_expr? gen_bool(addr);
        case INT: // emit(MOV [RSP+loc_addr], expect_tk("int")->value);
        case ARRAY:
            /* expr(next_tk(), 0, false, addr); */

    }
}

// ??? scuffed
// static void gen_array(long base_addr, enum DataType base_type,
//                       int dimension_count) {
// }

static void
gen_assign(struct Tk *p_ident_tk) // or addr
{
    struct Symbol *p_sym = sym_get(p_ident_tk);
    switch (p_sym->type) {
        case ARRAY: /* gen_array(p_sym->addr, p_sym->info.array.base_type,
                              p_sym->info.array.dimension_count); */
                    break;
        case STRUCT: // gen_struct(p_sym->addr); break;
        default: gen_expr_start(next_tk()); break;

    }
}

// NO DYNAMIC ARRAYS RN
// syntax: 'type' ARRAY['size0']['size1']['size..'] 'name'
static void gen_decl_array(enum DataType base_type) {
    int dimension_sizes[MAX_ARRAY_DIMENSION_COUNT]; // change 'int' to 'int64_t' as that's the type of int literals?
    int dimension_count = 0;
    // lex_next(&tk == BRACKET_L)
    while (next_tk()->type == BRACKET_L) {
        struct Tk *p_dimension_size;
        if ((p_dimension_size = next_tk())->type != LIT_INT) {
            tk_err(p_dimension_size, "Array dimension size expected 'int'");
        }

        // why is an int64_t being casted to int???
        dimension_sizes[dimension_count] = (int) tk.value.int_v;
        if (next_tk()->type != BRACKET_R) {
            tk_err(current_tk(), "Expected right square bracket ']'");
        }
        if (++dimension_count > MAX_ARRAY_DIMENSION_COUNT) {
            tk_err(current_tk(), "Cannot exceed array max dimension count of 5");
        }
    }

    struct Tk *p_tk_ident = current_tk();

    if (p_tk_ident->type != IDENT) {
        tk_err(p_tk_ident, "Expected identifier");
    }

    struct Symbol *p_sym = sym_push(VAR, p_tk_ident);
    memcpy(p_sym->info.array.dimension_sizes, dimension_sizes, sizeof(int));
    p_sym->info.array.dimension_count = dimension_count;
    p_sym->info.array.base_type = base_type;

    if (next_tk()->type != OP_AS) {
        // allocate garbage OR set to 0
        return;
    }

    // remove?
    next_tk();// remove?
    // assign
}


// handle scope
static void
gen_decl_scalar_var(enum DataType data_type, bool is_func_param) {
    struct Tk *p_tk_ident = current_tk();
    if (p_tk_ident->type != IDENT)
        tk_err(p_tk_ident, "Expected identifier");
    struct Symbol *p_sym = sym_push(VAR, p_tk_ident);
    p_sym->info.scalar_var.type = data_type;

    if (lex_next(&tk) != OP_AS) {
        // value is garbage
        // OR
        // gen_null(addr);
        return;
    }
    gen_assign(p_tk_ident);
}

/*
static void
gen_decl_array(void)
{
	// ARRAY['size0']['size1']['size..'] 'type' 'name'
	int dimension_sizes[MAX_ARRAY_DIMENSION_COUNT];
	int dimension_count = 0;
	while (lex_next(&tk == BRACKET_L)) {
		if (dimension_count++) > MAX_ARRAY_DIMENSION_COUNT) /* error ;
		if (lex_next(&tk) != INT) tk_err_fmt(bla bla bla);
		// this is sooooo
		dimension_sizes[dimension_count - 1] = (int) tk.value.int_v;
		if (lex_next(&tk) != BRACKET_R) tk_err_fmt(bla bla bla);
	}

	if (dimension_count == 0) /* error  ;
	enum DataType base_type = lex_next(
					data_type_from_kw(lex_next(&tk));
	if (base_type == ARRAY)
		tk_err(&tk, "Invalid base type");
	if (lex_next(&tk) != IDENT) {
        	tk_err(&tk, "Expected identifier");
   	}
	struct Symbol *p_sym = sym_push(VAR, &tk);
	memcpy(p_sym->info.array.dimension_sizes, dimension_sizes, sizeof(int))
	p_sym->info.array.dimension_count = dimension_count;
	p_sym->info.array.base_type = base_type;

	if (lex_next(&tk) != OP_AS) {
		// fill array
		return;
	}

	// assign
}

static void
gen_decl_struct(enum TkType kw_type)
{

}
*/

/* e.g. func bool asd(bool p1, int p2) {} */
/* can assume 'scope_level' is 0 when delcaring fn,
as nested fns *aren't* allowed */

static void
gen_decl_fn(void) // enum DataType ret_type)
{
    in_func = true;
    // struct Tk *p_ident_tk;


    enum DataType ret_type = data_type_from_kw(lex_next(&tk));
    if (ret_type == ARRAY || ret_type == STRUCT)
        tk_err(&tk, "Aggregate return type not allowed");


    if (lex_next(&tk) != IDENT) /* error */ ;
    struct Symbol *p_sym = sym_push(FUNC, &tk);
    p_sym->info.func.ret_type = ret_type;
    // use array instead of hashmap? hashmap_init(sym->info.func.params, /*random number*/ 4);
    // hashmap_init(sym->info.func.symbol_table);


    // params
    enum DataType param_data_type;
    struct Symbol *p_param_sym;
    for (int i = 0; next_tk()->type != PAREN_R; i++) {
        param_data_type = data_type_from_kw(lex_next(&tk));
        // if (param_data_type == -1) {// error}
        p_param_sym = gen_decl_var(param_data_type, true);
        // no error handling & soooo efficient
        p_sym->info.func.params_array =
           realloc(p_sym->info.func.params_array, sizeof(struct Symbol) * (i + 1));
        p_sym->info.func.params_array[i] = p_param_sym;
        // add checks for function calls preceding fn declaration
        if (lex_next(&tk) == COMMA) continue;
        if (tk.type == PAREN_R) break;
        tk_err(&tk, "Expected ')' or ','");
    }


    emit(ip); // label
    gen_block();
    if (current_tk()->type != SEMICOLON) /* error */ ;
    emit(RET);
    in_func = false;
}

// can_cast_type();
// TODO: finish
/* static void
gen_decl_fn(void) // enum DataType ret_type)
{
    in_func = true;
    // struct Tk *p_ident_tk;


    enum DataType ret_type = data_type_from_kw(lex_next(&tk));
    if (ret_type == ARRAY || ret_type == STRUCT)
        tk_err(&tk, "Aggregate return type not allowed");


    if (lex_next(&tk) != IDENT) // error;
    struct Symbol *p_sym = sym_push(FUNC, &tk);
    p_sym->info.func.ret_type = ret_type;
    // use array instead of hashmap? hashmap_init(sym->info.func.params, (random number) 4);
    // hashmap_init(sym->info.func.symbol_table);


    // params
    enum DataType param_data_type;
    struct Symbol *p_param_sym;
    for (int i = 0; next_tk()->type != PAREN_R; i++) {
        param_data_type = data_type_from_kw(lex_next(&tk));
        // if (param_data_type == -1) {// error}
        p_param_sym = gen_decl_var(param_data_type, true);
        // no error handling & soooo efficient
        p_sym->info.func.params_array =
           realloc(p_sym->info.func.params_array, sizeof(struct Symbol) * (i + 1));
        p_sym->info.func.params_array[i] = p_param_sym;
        // add checks for function calls preceding fn declaration
        if (lex_next(&tk) == COMMA) continue;
        if (tk.type == PAREN_R) break;
        tk_err(&tk, "Expected ')' or ','");
    }


    emit(ip); // label
    gen_block();
    if (current_tk()->type != SEMICOLON_R) // error ;
    emit(RET);
    in_func = false;
} */

// 'is_func_param': don't default value to 'null', instead let func call handle it
static struct Symbol *
gen_decl_var(enum DataType data_type, bool is_func_param) {
    switch (data_type) {
        // 'data_type' for case 'ARRAY' acts as base type array holds
        case ARRAY: gen_decl_array(data_type); break;
        case STRUCT: // gen_decl_struct(is_func_param); break; /* implied static type, unless dynamically resizable */
        default: gen_decl_scalar_var(data_type, is_func_param);
    }
    // ????
    // prob handle assignments here ig
    if (is_func_param) return NULL;
}

// handle things like "int fn foo()"
static void
set_data_type_info(union DataTypeInfo *data_type_info, struct Tk *p_type_kw_tk) {
    enum DataType data_type;
    switch (p_type_kw_tk->type) {
        case KW_BOOL: data_type = BOOL; break;
        case KW_CHAR: data_type = CHAR; break;
        case KW_INT: data_type = INT; break;
        case KW_NUM: data_type = NUM; break;
        case KW_STRING: data_type = STRING; break;
        case KW_ARRAY: data_type = ARRAY; break;
        case KW_STRUCT: data_type = STRUCT; break;
        default:
            tk_err(p_type_kw_tk, "wut happened");
    }


}

static void
gen_block(void) {
    struct Tk *p_tk = next_tk();
    for (;;) {
        switch (p_tk->type) {
            case IDENT:
                // gen_from_ident();
                break;
            case KW_BOOL:
            case KW_CHAR:
            case KW_INT:
            case KW_NUM:
            case KW_STRING:
            {
                union DataTypeInfo data_type_info;
                set_data_type_info(&data_type_info, p_tk);

                switch (next_tk()->type) {
                    case KW_ARRAY: break;
                    case KW_FN: break;
                    default:

                }
                break;
            }
            case KW_IF: // gen_if();
            // KW_ELIF: *handled in if*
            // KW_ELSE: *handled in if*
            case KW_WHILE: // gen_while();
            case KW_FOR: // gen_for();
            case KW_SWITCH: // gen_switch();
            case KW_JMP: // gen_jmp(); // not the instr
            case KW_FN: // gen_decl_fn();
                // case KW_LABEL: gen_decl_label();
            default:
                return;
        }
    }
}


uint8_t *
bytecode_gen_nofile(void) {
    /* init */
    hashmap_init(&symbol_table, HASHMAP_INIT_SIZE);

    // 'main.c' initialized lexer, maybe change that cuz a lil confusing
    // expr(next_tk(), 0, true);
    // printf("%d\n", tk.type);

    // struct Tk tk;
    lex_next(&tk);
    // tk.type != END
    // run once debug
     {
        switch (tk.type) {
            case IDENT:
                // gen_from_ident();
                break;
            case KW_BOOL:
            case KW_CHAR:
            case KW_INT:
            case KW_NUM:
            case KW_STRING:
            {
                int data_type = data_type_from_kw(tk.type);
                if (data_type == -1) /* error? */ ;

                if (next_tk()->type == KW_ARRAY)
                    gen_decl_array(data_type);
                else
                    gen_decl_scalar_var(data_type, false);
                //gen_decl_var(data_type_from_kw(tk.type), false);
                break;
            }
            case KW_IF: // gen_if();
            // KW_ELIF: *handled in if*
            // KW_ELSE: *handled in if*
            case KW_WHILE: // gen_while();
            case KW_FOR: // gen_for();
            case KW_SWITCH: // gen_switch();
            case KW_JMP: // gen_jmp(); // not the instr
            case KW_FN: // gen_decl_fn();
                // case KW_LABEL: gen_decl_label();
            default:


        }
    }

    // clear scope per iteration
    return NULL;
}

FILE *
bytecode_gen_file(void) {
    return NULL;
}
