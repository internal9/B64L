// TODO: to be overhauled
// 'LOAD' instr possibly needed
// Explicit values just to make stuff easier (yeah i gonna fix this later)
enum OPCODE
{
	// memory transfer operations
	MOVR = 0,	// # bytes used depends on reg used
	MOVSI = 1,	// move 64-bit integer value into stack address
	MOVSF = 100,	// move float value into stack address
	MOVH = 2,	// move value into heap address at stack address?
	TRS = 3,	// transfer to reg from stack
	TSR = 4,	// transfer to stack from reg
	TRH = 5,	// transfer to reg from heap address
	THR = 6,	// transfer to heap address from reg

	// arithmetic operations
	// op [reg]
	ZERO = 6,
	INC = 7,
	DEC = 8,

	//op fpreg, [value | fpreg]
	ADD = 9,
	SUB = 10,
	MUL = 11,
	DIV = 12,
	SQRT = 13,
	POW = 14,
	LOG = 15,

	// bitwise operations
	// op reg, [value | reg]
	NOT = 16,
	AND = 17,
	OR = 18,
	XOR = 19,
	BIC = 20,

	// logical shift (no regard for sign bit)
	LLSH = 21,
	LRSH = 22,

	// arithmetic shift (sign bit unaffected)
	ALSH = 23,
	ARSH = 24,

	// stack dedicated operations
	// op [val | reg]
	PUSH = 25,
	FPUSH = 26,

	// op [reg]
	POP = 26,
	FPOP = 27,	// popping into a general register performs double -> u64 conversion

	// branching operations
	JMP = 28,
	JNE = 29,
	JE = 30,
	JL = 31,
	JLE = 32,
	JG = 33,
	JGE = 34,
	CALL = 35,
	BICALL = 36,	// built-in call
	RET = 37,

	// dynamic memory operations
	/*
		alloc [addr], [bytes] heap-array addr to put memory address of heap-allocated data into
		has mode bit for zero-initializing newly allocated data IF resizing bigger
	*/
	ALLOC = 38,
	/*
		realloc [mem_addr], [bytes]
		mem_addr: addr of the heap-allocated data itself
	*/
	REALLOC = 39,
	/*
		dealloc [mem_addr]
		mem_addr: addr of the heap-allocated data itself
	*/
	DEALLOC = 40,

	// misc operations
	CMP = 41,
	PRINTREG = 42,
};
