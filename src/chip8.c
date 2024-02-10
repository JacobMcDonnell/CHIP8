#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROGRAMSTART	0x200

static uint16_t pop(void);
static int push(uint16_t);
static int readOp(uint16_t);
static void getArguments(uint16_t, uint8_t *, uint8_t *);

/* 16 8-bit registers */
uint8_t V[16] = {0};

/* program counter 0x200 is starting addr for programs */
uint16_t pc = PROGRAMSTART;

/* Index register: used for memory operations */
uint16_t I = 0;

uint16_t stack[16] = {0};
uint8_t stackPointer = 0;


/* main memory: 4096 bytes
 * 0x000 - 0x080: Font Set
 * 0x081 - 0x1FF: Interpreter
 * 0x200 - 0xFFF: Program Space */
uint8_t memory[4096] = {0};

static const uint8_t sprites[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void initCpu(void)
{
	/* Copy the sprites into the first 80 bytes into memory */
	memcpy(memory, sprites, sizeof(sprites));
	srand((unsigned)time(NULL));
}

int loadRom(char *file)
{
	FILE *fp = fopen(file, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Error opening file %s\n", file);
		exit(1);
	}
	fread(memory + PROGRAMSTART, 1, sizeof(memory) - PROGRAMSTART, fp);

	fclose(fp);
	return 0;
}

int cycleCpu(void)
{
	if (pc >= 4096)
		return 0;
	uint16_t op = 0;
	uint8_t upper = memory[pc], lower = memory[pc + 1];
	op = upper << 8;
	op = op | lower;
	readOp(op);
	printf("%3x: %4x\n", pc, op);
	pc += 2;
	return cycleCpu();
}

static int readOp(uint16_t opcode)
{
	uint8_t compare, reg;
	switch ((opcode >> 12)) {
		case 0x0:
			switch (opcode) {
				case 0x00E0: 	/* Clear the display */
					break;
				case 0x00EE:
					pc = pop(); /* Return to value at the top of the stack */
					break;
			}
			break;
		case 0x1:
			/* 1nnn: jump to addr nnn */
			pc = 0x0FFF & opcode;
			break;
		case 0x2:
			/* 2nnn: call addr nnn, save pc */
			if (push(pc) == 1) {
				return 1;
			}
			pc = 0x0FFF & opcode;
			break;
		case 0x3:
			/* 3xkk: skip next instruction if V[x] == kk */
			compare = opcode,
			reg = (0x0F00 & opcode) >> 8;
			pc += (compare == reg) ? 2 : 0;
			break;
		case 0x4:
			/* 4xkk: Skip next instruction if V[x] != kk */
			compare = opcode;
			reg = (0x0F00 & opcode) >> 8;
			pc += (compare != reg) ? 2 : 0;
			break;
		case 0x5:
			/* 5xy0: Skip next instruction if V[x] == V[y] */
			getArguments(opcode, &reg, &compare);
			pc += (compare == reg) ? 2 : 0;
			break;
		case 0x6:
			/* 6xkk: set V[x] = kk */
			reg = (opcode & 0x0F00) >> 8;
			compare = opcode;
			V[reg] = opcode;
			break;
		case 0x7:
			/* 7xkk: V[x] = V[x] + kk */
			reg = (opcode & 0x0F00) >> 8;
			compare = opcode;
			V[reg] += compare;
			break;
		case 0x8:
			getArguments(opcode, &reg, &compare);
			switch (opcode & 0x000F) {
				case 0x0:
					/* 8xy0: set V[x] = V[y] */
					V[reg] = V[compare];
					break;
				case 0x1:
					/* 8xy1: V[x] = V[x] | V[y] */
					V[reg] |= V[compare];
					break;
				case 0x2:
					/* 8xy2: V[x] = V[x] & V[y] */
					V[reg] &= V[compare];
					break;
				case 0x3: /* 8xy3 */
					V[reg] ^= V[compare];
					break;
				case 0x4: /* 8xy4 */
					/* V[0xF] marks an overflow */
					V[0xF] = ((V[reg] + V[compare] - 0xFFFF) > 0) ? 1 : 0;
					V[reg] += V[compare];
					break;
				case 0x5: /* 8xy5 */
					/* V[0xF] marks an underflow */
					V[0xF] = (V[reg] >= V[compare]) ? 1 : 0;
					V[reg] -= V[compare];
					break;
				case 0x6: /* 8xy6 */
					V[0xF] = 0x1 & V[reg];
					V[reg] = V[reg] << 1;
					break;
				case 0x7: /* 8xy7 */
					/* V[0xF] marks an underflow */
					V[0xF] = (V[reg] <= V[compare]) ? 1 : 0;
					V[reg] = V[compare] - V[reg];
					break;
				case 0xE:
					V[0xF] = 0x1 & V[reg];
					V[reg] = V[reg] >> 1;
					break;
				default:
					fprintf(stderr, "Error: Unkown Opcode %x\n", opcode);
					return 1;
			}
			break;
		case 0x9:
			/* 9xy0: Skip the next instruction if V[x] != V[y] */
			getArguments(opcode, &reg, &compare);
			pc += (V[reg] != V[compare]) ? 2 : 0;
			break;
		case 0xA:
			/* Annn: set I to addr nnn */
			I = opcode & 0x0FFF;
			break;
		case 0xB:
			/* Bnnn: program counter = nnn + V[0] */
			pc = (opcode & 0x0FFF) + V[0];
			break;
		case 0xC:
			reg = (opcode & 0x0F00) >> 8;
			compare = opcode & 0x00FF;
			V[reg] = rand() & compare;
			break;
		case 0xD:
			break;
		case 0xE:
			break;
		case 0xF:
			break;
		default:
			fprintf(stderr, "Error: Unknown Opcode %x\n", opcode);
			return 1;
	}
	return 0;
}

/* pop: pop a value from the stack */
static uint16_t pop(void)
{
	uint16_t stackValue = stack[stackPointer];
	stackPointer -= (stackPointer > 0) ? -1 : 0;
	return stackValue;
}

/* push: push a value to the stack */
static int push(uint16_t value)
{
	if (stackPointer >= 16 ) {
		fprintf(stderr, "Error: Stack Full\n");
		return 1;
	}
	stack[stackPointer++] = value;
	return 0;
}

/* getArguments: Get the first and second arguments from an opcode */
static void getArguments(uint16_t opcode, uint8_t *arg1, uint8_t *arg2)
{
	*arg1 = (opcode & 0x0F00) >> 8;
	*arg2 = (opcode & 0x00F0) >> 4;
}
