// TODO: to be overhauled
// 'LOAD' instr possibly needed
// Explicit values just to make stuff easier (yeah i gonna fix this later)
enum OPCODE
{
	// memory transfer operations
	OP_MOVR = 0,		// # bytes used depends on reg used
	OP_MOVSI = 1,		// move integer value into stack address (addr: base + index * scale + displacement)
	OP_MOVSIS = 900, 	// move integer value into stack address simple (addr: specified by one reg / value)
	OP_MOVSF = 100,	// move float value into stack address (addr: base + index * scale + displacement)
	OP_MOVSFS = 1234,	// move float value into stack address simple (addr: specified by one reg / value)
	OP_MOVH = 2,		// move value into heap address at stack address?
	OP_TRS = 3,		// transfer to reg from stack
	OP_TSR = 4,		// transfer to stack from reg
	OP_TRH = 5,		// transfer to reg from heap address
	OP_THR = 6,		// transfer to heap address from reg

	// arithmetic operations
	// op [reg]
	OP_ZERO = 6,
	OP_INC = 7,
	OP_DEC = 8,

	//op fpreg, [value | fpreg]
	OP_ADD = 9,
	OP_SUB = 10,
	OP_MUL = 11,
	OP_DIV = 12,
	OP_SQRT = 13,
	OP_POW = 14,
	OP_LOG = 15,

	// bitwise operations
	// op reg, [value | reg]
	OP_NOT = 16,
	OP_AND = 17,
	OP_OR = 18,
	OP_XOR = 19,
	OP_BIC = 20,

	// logical shift (no regard for sign bit)
	OP_LLSH = 21,
	OP_LRSH = 22,

	// arithmetic shift (sign bit unaffected)
	OP_ALSH = 23,
	OP_ARSH = 24,

	// stack dedicated operations
	// op [val | reg]
	OP_PUSH = 25,
	OP_FPUSH = 26,

	// op [reg]
	OP_POP = 26,
	OP_FPOP = 27,	// popping into a general register performs double -> u64 conversion

	// branching operations
	OP_JMP = 28,
	OP_JNE = 29,
	OP_JE = 30,
	OP_JL = 31,
	OP_JLE = 32,
	OP_JG = 33,
	OP_JGE = 34,
	OP_CALL = 35,
	OP_BICALL = 36,	// built-in call
	OP_RET = 37,

	// dynamic memory operations
	/*
		alloc [addr], [bytes] heap-array addr to put memory address of heap-allocated data into
		has mode bit for zero-initializing newly allocated data IF resizing bigger
	*/
	OP_ALLOC = 38,
	/*
		realloc [mem_addr], [bytes]
		mem_addr: addr of the heap-allocated data itself
	*/
	OP_REALLOC = 39,
	/*
		dealloc [mem_addr]
		mem_addr: addr of the heap-allocated data itself
	*/
	OP_DEALLOC = 40,

	// misc operations
	OP_CMP = 41,
	OP_PRINTREG = 42,
};

// Specify size to use for operands that can either be regs or literals, to save bytes ig. Orrr just read a reg
enum OPERAND_READ_MODE
{
	MODE_USE_REG = 0,
	
	/*
		If instructions operates on reg or stack location:
			 They expect sizeof(int64_t) or sizeof(double) amount of bits,
			 *depending* on if a reg or floating-point reg is being used
	
		If instruction can use literals:
			They expect sizeof(int64_t) or sizeof(double) amount of bits
			*depending* on instruction
	*/
	MODE_EIGHT = 1,
	MODE_HALF = 2,
	MODE_FULL = 3,

};

enum REG
{	
	REG_R1 = 0,
	REG_R2 = 1,
	REG_RAX = 2,
	REG_RBP = 3,
	REG_PRBP = 4,
	REG_RSP = 5,
	REG_FR1 = 6,
	REG_FR2 = 7
};
