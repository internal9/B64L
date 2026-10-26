/* NOTES:
 * - if a bytecode out file is being written to, & errors occurs, said file remains with the unfinished bytecode lol
 */

/* TODO:
 * - clean fclose handling of 'bytecode_out_file' if emit fails
 * - make it so all functions can call next_tk, use peeking
 * - Still need to standard 'lex_next' & 'next_tk' & 'current_tk'
 * - TBD: simplify 'next_tk' to just a current tk & next / peeked tk, for pushback features
 */

// INT ARRAY name[size1][size2]
// lex literals (e.g. TK_LIT_BOOL)
// init lexing here instead of in main.c?
#include "util.h"
#include "lex.h"
#include "VM.h"
#include "hashmap.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// static struct Tk tk;
// TBD: static struct Tk buf[some arbitrary num like 512];

// emit(MOVSI, st_addr(), rm(bsize(arg)), arg)

#define MAX_ARRAY_DIMENSION_COUNT 5

enum DataType {
    BOOL,
    CHAR,
    INT,	// remember 'TK_LIT_INT' exists too
    NUM,
    STRING,
	// incomplete types, use base (above) types to make a full type
    ARRAY,
    STRUCT
};

enum SymType {
    LABEL,
    VAR,
    FUNC,
};

/*
	excluding storage class specifiers like 'dynamic', 'static'
	things like 'int array[5]', 'struct A', 'struct {int a; num b}'
*/
struct CompleteDataTypeInfo {
	union {
	    struct {
	        enum DataType base_type;
	        int dimension_sizes[MAX_ARRAY_DIMENSION_COUNT]; // max dimension count
	        int dimension_count;
	    } ty_array;
		struct {
			bool is_ref;

		} ty_struct;	// 'struct Name'
		struct {

		} ty_struct_def;	// 'struct {members}'
	} meta_info;
	enum DataType type;
};

// find way to handle scope
// super "efficient"
struct Symbol {
    // TODO: get rid of the union?
	union {
		struct CompleteDataTypeInfo complete_data_type_info;
		// might simplify structs -> one var if applicable
		struct {
			struct CompleteDataTypeInfo return_data_type_info;
		} func;	// no aggregate return
		struct {
		} label;
	} info;
	const char* ident;
    enum SymType type;
    long line, column;
    long addr;
    // bool in_func;
    int block_id;   // one block id for global scope & for each fn?
    int scope_level;
};

// forward reference to a fn, calling a fn that doesn't exist (presumbly declared later / typo / doesn't exist)
struct ForwardFnCall {
    long line, column;
    struct Symbol** params_array;
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

static void gen_decl_var(struct CompleteDataTypeInfo *p_complete_data_type_info);
// static bool gen_expr(struct Tk *p_left, int prec_limit, bool is_expr_start);
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


static bool compile_error = false;

// TODO later: simplify
#define err_fmt(fmt, ...) (fprintf(stderr, fmt "\n", __VA_ARGS__), (compile_error = 1))
#define err(msg) 		  (fprintf(stderr, msg "\n"), (compile_error = true))
	
#define err_at(line, column, msg) 		   (fprintf(stderr, "(L%ld C%ld) " msg "\n", line, column), \
                            	  		   (compile_error = true))
#define err_fmt_at(line, column, fmt, ...) (fprintf(stderr, "(L%ld C%ld) " fmt "\n", \
                                            line, column, __VA_ARGS__), \
                                           (compile_error = true))

// passing pointer prob a lil unnecessary
#define tk_err(p_tk, msg) 				   err_at(p_tk->line, p_tk->column, msg);
#define tk_err_fmt(p_tk, fmt, ...) 		   err_fmt_at(p_tk->line, p_tk->column, fmt, __VA_ARGS__);


static FILE* bytecode_out_file = NULL;	// not 'NULL' with flags '-c' or '-ro
static uint8_t* bytecode_buffer = NULL;	// not 'NULL' with flag '-r'
static long bytecode_buffer_size = 0;	// in # of 'uint8_t's

/*
	also used to index bytecode buf,
	otherwise just used for addrs while file offset increases on its own
*/
static long prog_counter = 0;
/* static int64_t rsp = -1; ? */

// NOTE: prog_counter at end of bytecode compilation == 'bytecode_size'
// TODO: free memory & bytecode file on error
// NOTE: too lazy to check overflow for 'long'
static void
emit(uint8_t byte) {
	if (bytecode_out_file == NULL) {
		if (putc(byte, bytecode_out_file) == EOF)
			// TODO: better error handling,
			PERREXIT("Failed to write byte to bytecode out file");
	}
	else {
		// TODO: check this check
		if (prog_counter > (bytecode_buffer_size - 1))
			if ((bytecode_buffer = realloc(bytecode_buffer, bytecode_size * 2)) = NULL)
				PERREXIT("Failed to resize bytecode during compilation");
		bytecode_buffer[prog_counter++] = byte;
	}
}

static void
emit_n(int n, uint8_t bytes[n]) {
	// temp
	if (n < 1) PERREXIT("n cannot < 1");
	if (bytecode_out_file == NULL) {
		if (fwrite(bytes, sizeof(uint8_t), n, bytecode_out_file) != n)
			PERREXIT("Failed to write to bytecode out file during compilation");
	}
	else {
		// TODO: check this check
		if ((prog_counter + (n-1)) > (bytecode_buffer_size - 1))
			if ((bytecode_buffer = realloc(bytecode_buffer, bytecode_size * 2)) = NULL)
				PERREXIT("Failed to resize bytecode during compilation");
		memcpy(bytecode_buffer + prog_counter, bytes, n);
		prog_counter += n-1;
	}
}

#define INSTR_ARG_REG(opcode) (uint8_t) (0x80 | opcode) // if two args, arg2 is reg, if one arg, arg is reg
#define INSTR_ARG_LIT(opcode) (uint8_t) (0x00 | opcode)

static struct Symbol*
sym_push(enum SymType type, struct Tk *p_ident_tk) {
    /* pls arena alloc... or just stack allocate */
    struct Symbol *p_sym = malloc(sizeof(struct Symbol));
    p_sym->TK_IDENT = p_ident_tk->value.txt; // valid as long as tk remains in tk buffer
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

static struct Symbol*
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

static inline int op_prec(enum TkType tk_type) {
    switch (tk_type) {
        case TK_OP_ADD:
        case TK_OP_SUB: return 1;
        case TK_OP_MUL:
        case TK_OP_DIV: return 2;
        case TK_OP_POW: return 3;
        default: return -1;
    }
}

// temp debug
static void instr_debug(enum TkType type) {
    switch (type) {
        case TK_OP_ADD: puts("ADD R1, R2");
            break;
        case TK_OP_SUB: puts("SUB R1, R2");
            break;
        case TK_OP_MUL: puts("MUL R1, R2");
            break;
        case TK_OP_DIV: puts("DIV R1, R2");
            break;
        case TK_OP_POW: puts("POW R1, R2");
            break;
        default:		puts("instr_debug: 'type' is not an op tk");
    }
}


static int
data_type_from_kw(enum TkType TK_KW_type) {
    switch (TK_KW_type) {
        case TK_KW_BOOL: 	return BOOL;
        case TK_KW_CHAR: 	return CHAR;
        case TK_KW_INT: 	return INT;
        case TK_KW_NUM: 	return NUM;
        case TK_KW_STRING: 	return STRING;
        case TK_KW_ARRAY: 	return ARRAY;
        case TK_KW_STRUCT: 	return STRUCT;
        default: 			return -1;
    }
}

// static struct Tk buf[512];

// unnamed aggregate objects allowed (e.g. func((int) {'a', 2, 5}); func({.asd = 20})

// TODO: generalize function so it can be used for reading array assigning e.g. "arr[x][y] = val", mayb 'read_array_indexes / subscripts'
// TODO: omitted dimension sizes -> determined via member size at declaration & initialization e.g. int array[] x = {2, 4}, (int array[][]) {{5}}
#define OMITTED_DIMENSION_SIZE -1

static void
read_array_dimensions(struct CompleteDataTypeInfo *p_complete_data_type_info) {
	while (next_tk()->type == TK_BRACKET_L) {
        struct Tk *p_dimension_size;
        if ((p_dimension_size = next_tk())->type != TK_LIT_INT) {
            tk_err(p_dimension_size, "Array dimension size expected 'int'");
        }


        // why is an int64_t being casted to int???
        p_complete_data_type_info->meta_info.ty_array.dimension_sizes[
                p_complete_data_type_info->meta_info.ty_array.dimension_count
            ] = (int) tk.value.int_v;
        if (next_tk()->type != TK_BRACKET_R) {
            tk_err(current_tk(), "Expected right square bracket ']'");
        }
        if (++p_complete_data_type_info->meta_info.ty_array.dimension_count > MAX_ARRAY_DIMENSION_COUNT) {
            tk_err(current_tk(), "Cannot exceed array max dimension count of 5");
        }
    }
}


// TODO: handle things like "int fn foo()"
// e.g. compound literals: func(  (struct P) {1, 2, 5, 8}, (bool array[2]) {this, that}  );
static void
read_complete_data_type(struct CompleteDataTypeInfo *p_complete_data_type_info, struct Tk *p_type_TK_KW_tk) {
    enum DataType data_type = data_type_from_kw(p_type_TK_KW_tk->type);
    if (data_type == ARRAY)
        tk_err(p_type_TK_KW_tk, "Aggregate data types can only be specified after scalar");

	if (data_type == STRUCT) {
		p_complete_data_type_info->type = STRUCT;
	}
	else {
		bool is_array = data_type_from_kw(next_tk()->type) == ARRAY;
		if (is_array) {
			// Array Type
        	p_complete_data_type_info->type = ARRAY;
			read_array_dimensions(p_complete_data_type_info);
   		}
		else {
			// Scalar Type
			p_complete_data_type_info->type = data_type;
		}
	}
}

static void
read_data_type_decl() {
	// 'read_complete_data_type' and storage specifiers
}

static void
cast()
{

}

// '(type)' forms a cast operator
#define CAST_OP_PREC 4

/*
  Pratt-parser but...
  probably the worst thing written here
*/
/*
  Returns 'true' if it compiled an expression e.g 'a+b',
  rather than a standalone literal e.g. '2'
*/
// 'p_expr_data_type' is final type of expr after implicit casts e.g. 'int + float' -> expr evaluates to value of type 'float'
// TODO: NULL check p_expr_result_data_type maybe
// TBD: make another version using 255 regs like lua does, prob simpler
static bool
gen_expr(struct Tk *p_left, int prec_limit, bool is_expr_start, struct CompleteDataTypeInfo *p_expr_result_data_type) {
    static bool R1_use = false;

    // temp
    if (p_left->type == TK_END) // TODO: handle unexpected tks too
        puts("Expected expression"), exit(1);

    bool is_left_expr = false;
    switch (p_left->type) {
        case TK_LIT_TRUE: // emit(0b10000000 | MOV); emit(1); //emit("MOV R1, 1")
        case TK_LIT_FALSE:
        case TK_LIT_CHAR:
        case TK_LIT_INT: break;
        case TK_LIT_NUM:
        case TK_LIT_STR:
        case TK_PAREN_L: {
			// if any data type kw, assume explicit cast (unary expr) (prob scalar only for now) OR compound literal e.g. '(int)'
			if (data_type_from_kw(next_tk()->type != -1)) {
				struct CompleteDataTypeInfo complete_data_type_info;
				read_complete_data_type(&complete_data_type_info, current_tk());
				switch (complete_data_type_info.type) {
					// TBD: allow cast 'int' &/or 'char' to 'string'?
					case STRING: tk_err(current_tk(), "Explicit cast to string not allowed");
					case ARRAY: tk_err(current_tk(), "Array compound literal unimplemented");
					case STRUCT: tk_err(current_tk(), "Struct compound literal unimplemented");
					// Cast
					default: {
						if (next_tk()->type != TK_PAREN_R) tk_err(current_tk(), "Expected enclosing ')' for explicit cast");
						*p_expr_result_data_type = complete_data_type_info;
						// gen_expr(CAST_OP_PREC);
					}
				}
			}
			else {
			    struct CompleteDataTypeInfo left_expr_result_data_type;
	            is_left_expr = gen_expr(next_tk(), 0, false, &left_expr_result_data_type);
	            if (!is_left_expr)
	                p_left = current_tk(); // i hate it, since only literal retrieve that
	            struct Tk *p_tk = next_tk();
	            if (p_tk->type != TK_PAREN_R)
	                tk_err(p_tk, "Expected ')'");
			}
		}
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
        struct CompleteDataTypeInfo right_expr_result_data_type;
        bool is_right_sub_expr = gen_expr(next_tk(), prec, false, &right_expr_result_data_type);
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

    // TODO: decide about this
    if (is_expr_start) {
        // memcpy(&tk, next_tk(), sizeof(struct Tk));
        // tk_buf_clear();
    }
    return true;
}

#define gen_expr_start(p_left_tk, p_expr_result_data_type) gen_expr(p_left_tk, 0, true, p_expr_result_data_type)
// TODO: account for floating-points later, cuz fp regs


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

// or just pass in 'p_sym' lol
// TBD
static void
gen_assign(struct Tk *p_TK_IDENT_tk)
{
    struct Symbol *p_sym = sym_get(p_TK_IDENT_tk);
	if (p_sym == NULL)
		tk_err(p_TK_IDENT_tk, "TK_IDENTifier sym doesn't exist");

    switch (p_sym->type) {
		case LABEL: tk_err(p_TK_IDENT_tk, "Cannot assign to label");
		case FUNC: tk_err(p_TK_IDENT_tk, "Cannot assign to function");
        case ARRAY: /* gen_array(p_sym->addr, p_sym->info.array.base_type,
                              p_sym->info.array.dimension_count); */
                    break;
        case STRUCT: // gen_struct(p_sym->addr); break;
        default: {
			// scalar
			struct CompleteDataTypeInfo expr_result_data_type;
			gen_expr_start(next_tk(), &expr_result_data_type);
            // TODO: func that handles array compare, etc
			//if (expr_result_data_type != p_sym->info.complete_data_type_info)
				//cast("RAX", expr_result_data_type, cast_to_data_type);	// can error
			// emit("MOV [RBP+offset], RAX");
		}
    }
}

// TBD
static void
gen_assign_array()
{

}

// NO DYNAMIC ARRAYS RN
// syntax: 'type' ARRAY['size0']['size1']['size..'] 'name'
static void
gen_decl_array(struct CompleteDataTypeInfo* p_complete_data_type_info) {
    struct Tk *p_tk_TK_IDENT = current_tk();
    if (p_tk_TK_IDENT->type != TK_IDENT) {
        tk_err(p_tk_TK_IDENT, "Array: Expected TK_IDENTifier");
    }

    struct Symbol *p_sym = sym_push(VAR, p_tk_TK_IDENT);
    p_sym->info.complete_data_type_info = *p_complete_data_type_info;

    if (next_tk()->type != TK_OP_AS) {
        // allocate garbage OR set to 0
        return;
    }

	gen_assign(p_tk_TK_IDENT);
    // remove?
    next_tk();// remove?
}


// handle scope
static void
gen_decl_scalar_var(struct CompleteDataTypeInfo* p_complete_data_type_info) {
    struct Tk *p_TK_IDENT_tk = next_tk();
    if (p_TK_IDENT_tk->type != TK_IDENT)
        tk_err(p_TK_IDENT_tk, "Expected TK_IDENTifier");

    struct Symbol *p_sym = sym_push(VAR, p_TK_IDENT_tk);
    p_sym->info.complete_data_type_info = *p_complete_data_type_info;

    if (next_tk()->type != TK_OP_AS) {
        // value is garbage
        // OR
        // gen_null(addr);
        return;
    }

    gen_assign(p_TK_IDENT_tk);
}

static inline void
gen_decl_var(struct CompleteDataTypeInfo* p_complete_data_type_info) {
    if (p_complete_data_type_info->type == ARRAY)
        gen_decl_array(p_complete_data_type_info);
    else if (p_complete_data_type_info->type == STRUCT)
        ;	// gen_decl_struct(p_complete_data_type_info->meta_info.ty_struct);
    else
        gen_decl_scalar_var(p_complete_data_type_info);
}

/* e.g. func bool asd(bool p1, int p2) {} */
/* can assume 'scope_level' is 0 when delcaring fn,
as nested fns *aren't* allowed */

static void
gen_decl_fn(struct CompleteDataTypeInfo *p_return_data_type_info) // enum DataType ret_type)
{
    in_func = true;
    // struct Tk *p_TK_IDENT_tk;


    enum DataType ret_type = data_type_from_kw(lex_next(&tk));
    if (ret_type == ARRAY || ret_type == STRUCT)
        tk_err(&tk, "Aggregate return type not allowed");


    if (lex_next(&tk) != TK_IDENT) /* error */ ;
    struct Symbol *p_sym = sym_push(FUNC, &tk);
    p_sym->info.func.return_data_type_info = *p_return_data_type_info;
    // use array instead of hashmap? hashmap_init(sym->info.func.params, /*random number*/ 4);
    // hashmap_init(sym->info.func.symbol_table);


    // TODO: params
    // enum DataType param_data_type;
    // struct Symbol *p_param_sym;
    // for (int i = 0; next_tk()->type != PAREN_R; i++) {
    //
    // }
    //
    //
    // emit(ip); // label
    // gen_block();
    // if (current_tk()->type != SEMICOLON) /* error */ ;
    // emit(RET);
    // in_func = false;
}

// can_cast_type();

static void
gen_block(void) {
    struct Tk *p_tk = next_tk();
    for (;;) {
        switch (p_tk->type) {
            case TK_IDENT:
                // gen_from_TK_IDENT();
                break;
            case TK_KW_BOOL:
            case TK_KW_CHAR:
            case TK_KW_INT:
            case TK_KW_NUM:
            case TK_KW_STRING:
            {
                struct CompleteDataTypeInfo complete_data_type_info;
				read_complete_data_type(&complete_data_type_info, p_tk);
				gen_decl_var(&complete_data_type_info);
                break;
            }
            case TK_KW_IF: // gen_if();
            // TK_KW_ELIF: *handled in if*
            // TK_KW_ELSE: *handled in if*
            case TK_KW_WHILE: // gen_while();
            case TK_KW_FOR: // gen_for();
            case TK_KW_SWITCH: // gen_switch();
            case TK_KW_JMP: // gen_jmp(); // not the instr
            case TK_KW_FN: // gen_decl_fn();
                // case TK_KW_LABEL: gen_decl_label();
            default:
                return;
        }

        // just one iteration for a test
        break;
    }
}

/*
	!!!
	ISO C99: no good way to make file only if it doesn't exist:
	- fopen(file, "-r"):
		- file no exist? possible race conditions if file is created by smthn else
		- fails if no exist OR *no reading perm*, opening in latter case can't do write / read
	- would have to use POSIX / windows.h but too lazy

	!!!
	'w' flag will overwrite preexisting files
*/



uint8_t *
bytecode_gen_nofile(void) {
    /* init */
    hashmap_init(&symbol_table, HASHMAP_INIT_SIZE);
	if ((bytecode_buffer = malloc(256)) == NULL) PERREXIT("Failed to allocate for bytecode compilation");
	

    // 'main.c' initialized lexer, maybe change that cuz a lil confusing
    // expr(next_tk(), 0, true);
    // printf("%d\n", tk.type);

	gen_block();

    // TBD: clear scope per iteration
    return NULL;
}

FILE *
bytecode_gen_file(const char *out_file_name) {
	if ((bytecode_out_file = fopen(out_file_name, "wb")) == NULL) PERREXIT("Failed to make / open bytecode out file");
	/*

        if (fclose(bytecode_out_file) != 0) {
                free(bytecode_out_file);
                PERREXIT("Failed to close bytecode out file");
        }
	*/
	if (fclose(bytecode_out_file) != 0) PERREXIT("Failed to close bytecode out file");
    return bytecode_out_file;
}
