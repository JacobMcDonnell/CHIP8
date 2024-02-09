#include "chip8.h"

/* 16 8-bit registers */
uint8_t V[16] = {0};

/* program counter 0x200 is starting addr for programs */
uint16_t pc = 0x200;

uint16_t indexRegister = 0;

uint8_t stack[64] = {0};
uint8_t stackPointer = 0;


/* main memory: 4096 bytes
 * 0x000 - 0x080: Font Set
 * 0x081 - 0x1FF: Interpreter
 * 0x200 - 0xFFF: Program Space */
uint8_t memory[4096] = {0};


