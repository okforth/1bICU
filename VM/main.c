#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define OP_NOPO	0x0
#define OP_LD	0x1
#define OP_LDC	0x2
#define OP_AND	0x3
#define OP_ANDC	0x4
#define OP_OR	0x5
#define OP_ORC	0x6
#define OP_XNOR	0x7
#define	OP_STO	0x8
#define OP_STOC	0x9
#define	OP_IEN	0xA
#define OP_OEN	0xB
#define OP_JMP	0xC
#define OP_RTN	0xD
#define OP_SKZ	0xE
#define OP_NOPF	0xF

/* DATA BUS */
bool DATA;

/* INPUT PINS */
uint8_t INST;
bool RST;

/* OUTPUT PINS */
bool RR;
bool FLAG_WRITE;
bool FLAG_JMP;
bool FLAG_RTN;
bool FLAG_O;
bool FLAG_F;

/* INTERNAL REGISTERS */
bool FLAG_IEN;
bool FLAG_OEN;
bool SKIP;

void chip() {
	/* RESET FLAGS */
	FLAG_WRITE = 0;
	FLAG_JMP = 0;
	FLAG_RTN = 0;
	FLAG_O = 0;
	FLAG_F = 0;

	if (RST) {
		FLAG_IEN = 0;
		FLAG_OEN = 0;
		return;
	}

	if (SKIP) {
		SKIP = 0;
		return;
	}

	if (INST == OP_NOPO) {
		FLAG_O = 1;
		return;
	}

	if (INST == OP_LD) {
		RR = DATA;
		return;
	}

	if (INST == OP_LDC) {
		RR = !DATA;
		return;
	}

	if (INST == OP_AND) {
		RR = RR & DATA;
		return;
	}

	if (INST == OP_ANDC) {
		RR = RR & !DATA;
		return;
	}

	if (INST == OP_OR) {
		RR = RR | DATA;
		return;
	}

	if (INST == OP_ORC) {
		RR = RR | !DATA;
		return;
	}

	if (INST == OP_XNOR) {
		RR = !(RR ^ DATA);
		return;
	}

	if (INST == OP_STO) {
		if (FLAG_OEN) {
			DATA = RR;
			FLAG_WRITE = 1;
		}
		return;
	}

	if (INST == OP_STOC) {
		if (FLAG_OEN) {
			DATA = !RR;
			FLAG_WRITE = 1;
		}
		return;
	}

	if (INST == OP_IEN) {
		FLAG_IEN = DATA;
		return;
	}

	if (INST == OP_OEN) {
		FLAG_OEN = DATA;
		return;
	}

	if (INST == OP_JMP) {
		FLAG_JMP = 1;
		return;
	}

	if (INST == OP_RTN) {
		FLAG_RTN = 1;
		SKIP = 1;
		return;
	}

	if (INST == OP_SKZ) {
		if (RR == 0) {
			SKIP = 1;
		}
		return;
	}

	if (INST == OP_NOPF) {
		FLAG_F = 1;
		return;
	}
}

unsigned int status_count = 0;

void status() {
	printf("count: %d\n", status_count++);
	printf("DATA		%d\n", DATA);
	printf("INST		%X\n", INST);
	printf("RR		%d\n", RR);
	printf("RST		%d\n", RST);
	printf("FLAG_WRITE	%d\n", FLAG_WRITE);
	printf("FLAG_JMP	%d\n", FLAG_JMP);
	printf("FLAG_RTN	%d\n", FLAG_RTN);
	printf("FLAG_O		%d\n", FLAG_O);
	printf("FLAG_F		%d\n", FLAG_F);
	printf("FLAG_IEN	%d\n", FLAG_IEN);
	printf("FLAG_OEN	%d\n", FLAG_OEN);
	printf("SKIP		%d\n", SKIP);
	printf("\n");
}

void step(uint8_t inst, bool data, bool rst) {
	INST = inst;
	DATA = data;
	RST = rst;
	chip();
}

#define LOW		0x00
#define HIGH		0x01
#define ARG_A		0x10
#define ARG_B		0x11
#define ARG_C		0x12
#define ARG_XOR		0x13
#define ARG_AND		0x14

#define ARG_X0		0x30
#define ARG_X1		0x31
#define ARG_X2		0x32
#define ARG_X3		0x33

#define ARG_Y0		0x40
#define ARG_Y1		0x41
#define ARG_Y2		0x42
#define ARG_Y3		0x43

#define ARG_Z0		0x50
#define ARG_Z1		0x51
#define ARG_Z2		0x52
#define ARG_Z3		0x53


uint8_t ptr = 0;
uint8_t rtn;

bool ram[256] = {
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* occupied */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* arguments */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* results */
	1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* x */
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* y */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* z=x+y */
};

uint8_t rom[256] = {
	OP_IEN,  HIGH,		/* 0x00 */
	OP_OEN,  HIGH,		/* 0x02 */
	OP_JMP,  0x20,		/* 0x04 */

	/* FULL-ADDER SUBROUTINE */
	OP_LD,   ARG_A,		/* 0x06 */
	OP_XNOR, ARG_B,		/* 0x08 */
	OP_STOC, ARG_XOR,	/* 0x0A */ /* A ^ B */

	OP_LD,   ARG_A,		/* 0x0C */
	OP_AND,  ARG_B,		/* 0x0E */
	OP_STO,  ARG_AND,	/* 0x10 */ /* A & B */

	OP_LD,   ARG_XOR,	/* 0x12 */
	OP_XNOR, ARG_C,		/* 0x14 */
	OP_STOC, ARG_B,		/* 0x16 */ /* (A ^ B) ^ C */

	OP_LD,   ARG_XOR,	/* 0x18 */
	OP_AND,  ARG_C,		/* 0x1A */
	OP_OR,   ARG_AND,	/* 0x1C */
	OP_STO,  ARG_C,		/* 0x1E */ /* ((A ^ B) & C) | (A & B) */

	OP_RTN,  0,		/* 0x20 */

	/* LOAD */
	OP_LD,   ARG_X0,
	OP_STO,  ARG_A,
	OP_LD,   ARG_Y0,
	OP_STO,  ARG_B,
	OP_JMP,  0x04,
	OP_LD,   ARG_B,
	OP_STO,  ARG_Z0,

	OP_LD,   ARG_X1,
	OP_STO,  ARG_A,
	OP_LD,   ARG_Y1,
	OP_STO,  ARG_B,
	OP_JMP,  0x04,
	OP_LD,   ARG_B,
	OP_STO,  ARG_Z1,

	OP_LD,   ARG_X2,
	OP_STO,  ARG_A,
	OP_LD,   ARG_Y2,
	OP_STO,  ARG_B,
	OP_JMP,  0x04,
	OP_LD,   ARG_B,
	OP_STO,  ARG_Z2,

	OP_LD,   ARG_X3,
	OP_STO,  ARG_A,
	OP_LD,   ARG_Y3,
	OP_STO,  ARG_B,
	OP_JMP,  0x04,
	OP_LD,   ARG_B,
	OP_STO,  ARG_Z3,

	0
};

void show_ram() {
	for (int i = 0; i < 256; i++) {
		if (i % 16 == 0) printf("\n0x%X0:\t", i / 16);
		printf("%d", ram[i]);
	}
}

int main() {
	while (ptr < 254) {
		printf("ptr: %X\n", ptr);
		uint8_t inst = rom[ptr];
		uint8_t addr = rom[ptr+1];
		uint8_t data = ram[addr];
		step(inst, data, 0);
		if (FLAG_WRITE) {
			ram[addr] = DATA;
		} else if (FLAG_JMP) {
			rtn = ptr;
			ptr = addr;
		} else if (FLAG_RTN) {
			ptr = rtn;
			continue;
		}
		ptr += 2;
	}

	show_ram();

	return 0;
}
