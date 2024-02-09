#include <stdint.h>

extern uint8_t V[16];
extern uint16_t pc;
extern uint8_t stack[64];
extern uint8_t stackPointer;
extern uint8_t memory[4096];

void initCpu(void);
int loadRom(char *);
int cycleCpu(void);

