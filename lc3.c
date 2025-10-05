#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

// --- --- --- HARDWARE COMPONENTS --- --- --- ///
//                  START
// Memory for the computer - 128KB of RAM
#define MEMORY_MAX (1 << 16)
uint16_t memory[MEMORY_MAX]; // - 65,536 places in memory

// Registers for the CPU
enum {
    R_R0 = 0,   // general purpose register
    R_R1,       // general purpose register
    R_R2 ,      // general purpose register
    R_R3 ,      // general purpose register
    R_R4 ,      // general purpose register
    R_R5 ,      // general purpose register
    R_R6 ,      // general purpose register
    R_R7 ,      // general purpose register
    R_PC ,      // Program counter
    R_COND ,    // condition register
    R_COUNT ,   // Register counter
};

uint16_t reg[R_COUNT];

// Opcodes for the CPU
enum {
    OP_BR = 0,  // branch - 0000 -0
    OP_ADD,     // add - 0001 - 1
    OP_LD,      // load - 0010 - 2
    OP_ST,      // store - 0011 - 3
    OP_JSR,     // jump register - 0100 - 4
    OP_AND,     // bitwise and - 0101 - 5
    OP_LDR,     // load register - 0110 - 6
    OP_STR,     // store register - 0111 - 7
    OP_RTI,     // unused - 1000 - 8
    OP_NOT,     // bitwise not - 1001 - 9
    OP_LDI,     // load indirect - 1010 - 10
    OP_STI,     // store indirect - 1011 - 11
    OP_JMP,     // jump - 1100 - 12
    OP_RES,     // reserved (unused) 1101 - 13
    OP_LEA,     // load effective address 1110 - 14
    OP_TRAP,    // execute trap - 1111 - 15
}

// Condition flags for R_COND
enum {
    FL_POS = 1 << 0,    // P
    FL_ZRO = 1 << 1,    // Z
    FL_NEG = 1 << 2,    // N
};

// --- --- --- HARDWARE COMPONENTS --- --- --- ///
//                    END

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        // show usage string
        printf("lc3 [image-file1]..\n");
        exit(2);
    }

    for (int j = 1; j < argc; ++j) {
        if (!read_image(argv[j])) {
            printf("Failed to load image: %s\n", argv[j]);
            exit(1);
        }
    }

    // exactly one condition flag should be set at a any given time, thus we set the Z flag
    reg[R_COND] = FL_ZRO;

    // set the PC to starting position
    // 0x3000 is the default

    enum { PC_START = 0x3000 };
    reg[R_PC] = PC_START;
    
    int running = 1;
    while (running) {
        // FETCH
        uint16_t instr = mem_read(reg[R_PC]++); // instr = 0011 0000 0000 0000
        // R_PC = 0x3001
        uint16_t op = instr >> 12; // op = 0000 0000 0000 0011

        switch (op) {
            case OP_ADD:
            /* ENCODING FOR ADD INSTRUCTION
                      #Register mode 
                15_12|11_9|8__6|5|43|2____0
                |0001| DR | SR1|0|00| SR2 |

                Assm. ex.: ADD R2 R0 R1;  Add contents of R0 to R1 and store in R2

                --- --- --- --- --- ---

                      #Immediate mode
                15_12|11_9|8__6|5|4____0
                |0001| DR | SR1|1| imm5|

                Assm. ex: ADD R0 R0 1; add 1 to R0 and store back in R0

            */

                // destination register (DR)
                uint16_t r0 = (instr >> 9) & 0x7;
                /*
                    0011 0000 0000 0000 >> 9 = 0000 0000 0001 1000 (0x18) 24
                    0000 0000 0001 1000
                   &0000 0000 0000 0111
                   --------------------
                    0000 0000 0000 0000
                */
                // first operand (SR1)
                uint16_t r1 = (instr >> 6) & 0x7;
                /*
                    0011 0000 0000 0000 >> 6 = 0000 0000 1100 0000 (0xc0) 192
                    0000 0000 1100 0000
                   &0000 0000 0000 0011
                    -------------------
                    0000 0000 0000 0000
                */
                // whether we are in immediate mode
                uint16_t imm_flag = (instr >> 5) & 0x1;
                /*
                    0011 0000 0000 0000 >> 5 = 0000 0001 1000 0000 (0x180) 384
                    0000 0001 1000 0000
                   &0000 0000 0000 0001
                    -------------------
                    0000 0000 0000 0000
                */

                if (imm_flag) {
                    uint16_t imm5 = sign_extend(instr & 0x1F, 5);
                    reg[r0] = reg[r1] + imm5;
                }
                else {
                    uint16_t r2 = instr & 0x7;
                    reg[r0] = reg[r1] + reg[r2];
                }

                update_flags(r0);
                break;
            case OP_AND:
                break;
            case OP_NOT:
                break;
            case OP_BR:
                break;
            case OP_JMP:
                break;
            case OP_LSR:
                break;
            case OP_LD:
                break;
            case OP_LDI:
            /* LDI ENCODING
                15__12|11_9|8_________0
                | 1010| DR | PCoffset9|
            */

                // destination register (DR)
                uint16_t r0 = (instr >> 9) * 0x7;
                // PCOffset9
                uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
                //add pc_offset to current PC, look at memory location to get the final address
                reg[r0] = mem_read(mem_read(reg[R_PC] + pc_offset));
                update_flags(r0);
                break;
            case OP_LDR:
                break;
            case OP_LEA:
                break;
            case OP_ST:
                break;
            case OP_STI:
                break;
            case OP_STR:
                break;
            case OP_TRAP:
                break;
            case OP_RES:
            case OP_RTI:
                break;
            default:

                break;
        }
    }
}

/* 
    uses two's complement in order to do 'sign extending'.
    The wikipedia page explaining two's complement :
        https://en.wikipedia.org/wiki/Two%27s_complement
    I vaguely understand, but not enough to give an adequate description
    as of yet.
*/
uint16_t sign_extend(uint16_t x, int bit_count) {
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

void update_flags(uint16_t r) {
    if (reg[r] == 0) {
        reg[R_COND] = FL_ZRO;
    } 
    else if (reg[r] >> 15) {
        reg[R_COND] = FL_NEG; // a 1 in the left-most bit indicates negative.
                              // See the encodings of the opcode instructions
    } 
    else {
        reg[R_COND] = FL_POS;
    }

    /*
    -- Leaving this here as I thought this is what needed to happen.
       I had point, just a bit misguided. The correct code is up there

    else if (reg[r] > 0) {
        reg[R_COND] = FL_POS;
    }
    else if (reg[r] < 0) {
        reg[R_COND] = FL_NEG
    }
    */
}