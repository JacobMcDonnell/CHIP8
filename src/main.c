#include "chip8.h"
#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: chip8 rom.ch8\n");
		return 1;
	}
	initCpu();
	loadRom(argv[1]);
	cycleCpu();
	return 0;
}
