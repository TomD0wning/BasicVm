#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define STACK_SIZE 256

static int stack[STACK_SIZE];

typedef enum {
	HLT, // 0  -- hlt              :: halts program
	PSH, // 1  -- psh val          :: pushes <val> to stack
	POP, // 2  -- pop              :: pops value from stack
	ADD, // 3  -- add              :: adds top two vals on stack
	MUL, // 4  -- mul              :: multiplies top two vals on stack
	DIV, // 5  -- div              :: divides top two vals on stack
	SUB, // 6  -- sub              :: subtracts top two vals on stack
	SLT, // 7  -- slt reg_a, reg_b :: pushes (reg_a < reg_b) to stack
	MOV, // 8  -- mov reg_a, reg_b :: movs the value in reg_a to reg_b
	SET, // 9  -- set reg, val     :: sets the reg to value
	LOG, // 10 -- log a            :: prints out a
	IF,  // 11 -- if reg val ip    :: if the register == val branch to the ip
	IFN, // 12 -- ifn reg val ip   :: if the register != val branch to the ip
	GLD, // 13 -- gld reg          :: loads a register to the stack
	GPT, // 14 -- gpt reg          :: pushes top of stack to the given register
	NOP  // 15 -- nop              :: nothing
} Instructions;

typedef enum {
	A, B, C, D, E, F, I, J, // GENERAL PURPOSE
	EX,                     // EXCESS
	EXA,                    // MORE EXCESS
	IP,                     // INSTRUCTION POINTER
	SP,                     // STACK POINTER
	REGISTER_SIZE
} Registers;

static int registers[REGISTER_SIZE];

//instruction array
int* instructions;

// number of instructions to do
int instruction_count;

//allocated space per instruction
int instruction_space = 4;

// is program running
bool isRunning = true;

// if the IP is assigned by jmp instructions(such as IF,IFN),it should not increase 1 any more
bool isJmp = false;

// shortcuts to SP & IP
#define SP (registers[SP])
#define IP (registers[IP])

// get current instruction set
#define FETCH (registers[IP])

void print_newline(int i) {
	if ((i + 1) % 4 == 0) { printf("\n"); }
}

// print that stack
void print_stack() {
	for (int i = 0; i < SP; i++)
	{
		printf("0x%04d ", stack[i]);
		print_newline(i);
	}
}

void pr0int_registers() {
	printf("Register dump:\n");
	for (int i = 0; i < REGISTER_SIZE; i++)
	{
		printf("%04d ", registers[i]);
		print_newline(i);
	}
}

int find_empty_registers() {
	for (int i = 0; i < REGISTER_SIZE; i++)
	{
		if (i != registers[EX] && i != registers[EXA]) { return i; }
	}
	return EX;
}

void eval(int instr) {
	isJmp = false;
	switch (instr) {
	case HLT: {
		isRunning = false;
		printf("Finished Execution\n");
		// print_stack(0, 16);
		// print_registers();
		break;
	}
	case PSH: {
		SP = SP + 1;
		IP = IP + 1;
		stack[SP] = instructions[IP];
		break;
	}
	case POP: {
		SP = SP - 1;
		break;
	}
	case ADD: {
		registers[A] = stack[SP];
		SP = SP - 1;

		registers[B] = stack[SP];
		/* SP = SP - 1; */

		registers[C] = registers[B] + registers[A];

		/* SP = SP + 1; */
		stack[SP] = registers[C];
		printf("%d + %d = %d\n", registers[B], registers[A], registers[C]);
		break;
	}
	case MUL: {
		registers[A] = stack[SP];
		SP = SP - 1;

		registers[B] = stack[SP];
		/*SP = SP - 1;*/

		registers[C] = registers[B] * registers[A];

		/*SP = SP + 1;*/
		stack[SP] = registers[C];
		printf("%d * %d = %d\n", registers[B], registers[A], registers[C]);
		break;
	}
	case DIV: {
		registers[A] = stack[SP];
		SP = SP - 1;

		registers[B] = stack[SP];
		/* SP = SP - 1;*/

		registers[C] = registers[B] / registers[A];

		/* SP = SP + 1; */
		stack[SP] = registers[C];
		printf("%d / %d = %d\n", registers[B], registers[A], registers[C]);
		break;
	}
	case SUB: {
		registers[A] = stack[SP];
		SP = SP - 1;

		registers[B] = stack[SP];
		/* SP = SP - 1; */

		registers[C] = registers[B] - registers[A];

		/* SP = SP + 1; */
		stack[SP] = registers[C];
		printf("%d - %d = %d\n", registers[B], registers[A], registers[C]);
		break;
	}
	case SLT: {
		SP = SP - 1;
		stack[SP] = stack[SP + 1] < stack[SP];
		break;
	}
	case MOV: {
		registers[instructions[IP + 2]] = registers[instructions[IP + 1]];
		IP = IP + 2;
		break;
	}
	case SET: {
		registers[instructions[IP + 1]] = instructions[IP + 2];
		IP = IP + 2;
		break;
	}
	case LOG: {
		printf("%d\n", registers[instructions[IP + 1]]);
		IP = IP + 1;
		break;
	}
	case IF: {
		if (registers[instructions[IP + 1]] == instructions[IP + 2]) {
			IP = instructions[IP + 3];
			isJmp = true;
		}
		else {
			IP = IP + 3;
		}
		break;
	}
	case IFN: {
		if (registers[instructions[IP + 1]] != instructions[IP + 2]) {
			IP = instructions[IP + 3];
			isJmp = true;
		}
		else {
			IP = IP + 3;
		}
		break;
	}
	case GLD: {
		SP = SP + 1;
		IP = IP + 1;
		stack[SP] = registers[instructions[IP]];
		break;
	}
	case GPT: {
		registers[instructions[IP + 1]] = stack[SP];
		IP = IP + 1;
		break;
	}
	case NOP: {
		printf("NO-OP\n");
		break;
	}
	default: {
		printf("Unknown Instruction %d\n", instr);
		break;
	}
	}
}

int main(int argc, char** argv) {
	if (argc <= 1) {
		printf("error: no input\n");
		return -1;
	}

	//filename
	char* filename = argv[1];

	FILE* file = fopen(filename, "r");
	if (!file) {
		printf("error: could not read file `%s`\n", filename);
		return -1;
	}

	//alloc instruction space
	instructions = malloc(sizeof(*instructions) * instruction_space); //4 instructions

	//read bin files
	int num;
	int i = 0;
	while (fscan(file, "%d", &num) > 0)
	{
		instructions[i] = num;
		printf("%d\n", instructions[i]);
		i++;
		if (i >= instruction_space) {
			instruction_space *= 2;
			instructions = realloc(instructions, sizeof(*instructions) * instruction_space); // 2x space
		}
	}

	//set ins count to read instructions
	instruction_count = i;

	// close file
	fclose(file);

	//init stack pointer
	SP = -1;

	//run while still within instruction count
	while (isRunning && IP < instruction_count)
	{
		eval(FETCH);
		if (!isJmp) {
			IP = IP + 1;
		}
	};

	//clean up
	free(instructions);

	return 0;
}