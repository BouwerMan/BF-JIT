#define _GNU_SOURCE
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_FILE "num.bf"
#define DEFAULT_OUT_FILE "out.asm"
#define PROGRAM_SIZE 4096
#define STACK_SIZE 512
#define DATA_SIZE 128

enum op {
    OP_END,
    OP_INC_DP,
    OP_DEC_DP,
    OP_INC_VAL,
    OP_DEC_VAL,
    OP_OUT,
    OP_IN,
    OP_JMP_FWD,
    OP_JMP_BCK
};

struct instruction {
    unsigned short operator;
    unsigned short operand;
};

static struct instruction program[PROGRAM_SIZE] = { 0 };
static unsigned short inst_stack[STACK_SIZE];
static unsigned int stack_pointer = 0;

static char* filename = DEFAULT_FILE;
static char* out_filename = DEFAULT_OUT_FILE;
static bool debug = false;

// DONT USE THIS IT DOESNT REALLY WORK
int compile_program(FILE* file)
{
    printf("Trying to compile to %s\n", out_filename);
    // Setup syscall constants
    fprintf(file, "SYS_WRITE   equ 1\nSYS_EXIT    equ 60\nSTDOUT      equ 1\n\n");

    // TODO: Dynamically size array based on amount of OP_INC_DP
    fprintf(file, "SECTION .data\ndata_array times %d dd 0\n", DATA_SIZE);

    // Setup main func
    fprintf(file, "SECTION .text\nglobal _start\n_start:\n");

    unsigned int pc = 0, dp = 0;
    int peek_pc, val;

    while (program[pc].operator!= OP_END) {
        switch (program[pc].operator) {
        case OP_INC_DP:
            dp++;
            /**
            peek_pc = pc;
            val = 8;
            while (program[++peek_pc].operator== OP_INC_DP) {
                val += 8;
                pc++;
            }
            fprintf(file, "    sub rsp, %d\n", val);
            */
            break;
        case OP_DEC_DP:
            dp--;
            /**
            peek_pc = pc;
            val = 8;
            while (program[++peek_pc].operator== OP_DEC_DP) {
                val += 8;
                pc++;
            }
            fprintf(file, "    add rsp, %d\n", val);
            */
            break;
        case OP_INC_VAL:
            peek_pc = pc;
            val = 1;
            while (program[++peek_pc].operator== OP_INC_VAL) {
                val++;
                pc++;
            }
            fprintf(file, "    mov edi, DWORD [data_array + %d * 4]\n", dp);
            fprintf(file, "    add edi, %d\n", val);
            fprintf(file, "    mov [data_array + %d * 4], DWORD edi\n", dp);
            break;
        case OP_DEC_VAL:
            peek_pc = pc;
            val = 1;
            while (program[++peek_pc].operator== OP_DEC_VAL) {
                val++;
                pc++;
            }
            fprintf(file, "    mov edi, DWORD [data_array + %d * 4]\n", dp);
            fprintf(file, "    sub edi, %d\n", val);
            fprintf(file, "    mov [data_array + %d * 4], DWORD edi\n", dp);
            break;
        case OP_OUT:
            fprintf(file, "    ; OP_OUT\n");
            fprintf(file, "    mov rax, SYS_WRITE\n    mov rdi, STDOUT\n");
            fprintf(file, "    mov rsi, DWORD data_array + %d * 4\n    mov rdx, 1\n    syscall\n", dp);
            fprintf(file, "    ; OP_OUT end\n");
            break;
        case OP_JMP_FWD:
            fprintf(file, ".L%d:\n", pc);
            fprintf(file, "    mov eax, DWORD [data_array + %d * 4]\n", dp);
            fprintf(file, "    cmp eax, 0\n    je .L%d\n", program[pc].operand);
            break;
        case OP_JMP_BCK:
            fprintf(file, "    mov eax, DWORD [data_array + %d * 4]\n", dp);
            // fprintf(file, "    cmp edi, 0\n    jne .L%d\n", program[pc].operand);
            fprintf(file, "    cmp eax, 0\n    jne .L%d\n", program[pc].operand);
            fprintf(file, ".L%d:\n", pc);
            break;
        }
        pc++;
    }
    // linux exit
    fprintf(file, "\n");
    fprintf(file, "    mov rax, SYS_EXIT\n    mov rdi, [data_array + %d * 4]\n    syscall", dp);
    return 0;
}

void execute_program()
{
    printf("Executing interpreted program: %s\n", filename);
    int* data = calloc(DATA_SIZE, sizeof(int));
    unsigned int pc = 0, dp = 0;

    while (program[pc].operator!= OP_END) {
        switch (program[pc].operator) {
        case OP_INC_DP:
            dp++;
            break;
        case OP_DEC_DP:
            dp--;
            break;
        case OP_INC_VAL:
            data[dp]++;
            break;
        case OP_DEC_VAL:
            data[dp]--;
            break;
        case OP_OUT:
            putchar(data[dp]);
            break;
        case OP_IN:
            data[dp] = getchar();
            break;
        case OP_JMP_FWD:
            if (data[dp] == 0) {
                pc = program[pc].operand;
            }
            break;
        case OP_JMP_BCK:
            if (data[dp]) {
                pc = program[pc].operand;
            }
            break;
        default:
            fprintf(stderr, "Literally how did u get here? Unkown op code found\n");
            exit(EXIT_FAILURE);
        }
        pc++;
    }

    if (debug) {
        printf("\nData stack:\n");
        for (size_t i = 0; i <= dp; i++) {
            printf("    data[%lu]: 0x%x\n", i, data[i]);
        }
    }
    free(data);
}

int interpret_program(FILE* file)
{
    printf("Interpreting %s\n", filename);
    unsigned int pc = 0, jmp_pc;
    int c;
    while ((c = fgetc(file)) != EOF && pc < PROGRAM_SIZE) {
        switch ((char)c) {
        case '>':
            program[pc].operator= OP_INC_DP;
            break;
        case '<':
            program[pc].operator= OP_DEC_DP;
            break;
        case '+':
            program[pc].operator= OP_INC_VAL;
            break;
        case '-':
            program[pc].operator= OP_DEC_VAL;
            break;
        case '.':
            program[pc].operator= OP_OUT;
            break;
        case ',':
            program[pc].operator= OP_IN;
            break;
        case '[':
            if (stack_pointer == STACK_SIZE) return 1; // Fail if no stack left
            program[pc].operator= OP_JMP_FWD;
            inst_stack[stack_pointer++] = pc;
            break;
        case ']':
            if (stack_pointer == STACK_SIZE) return 1; // Fail if no stack left
            jmp_pc = inst_stack[--stack_pointer];
            program[pc].operator= OP_JMP_BCK;
            program[pc].operand = jmp_pc;
            program[jmp_pc].operand = pc;
            break;
        default:
            pc--;
            break;
        }
        pc++;
    }

    if (pc == PROGRAM_SIZE) {
        fprintf(stderr, "Program too large!\n");
        return 1;
    } else if (stack_pointer) {
        fprintf(stderr, "Unable to find closing bracket\n");
        return 1;
    }
    program[pc].operator= OP_END;
    return 0;
}

int main(int argc, char* argv[])
{
    bool interpret = true;
    char c;
    while ((c = getopt(argc, argv, "f:do:")) != -1) {
        switch (c) {
        case 'f':
            filename = optarg;
            break;
        case 'd':
            debug = true;
            break;
        case 'o':
            out_filename = optarg;
            interpret = false;
            break;
        case '?':
            if (optopt == 'f')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr,
                    "Unknown option character `\\x%x'.\n",
                    optopt);
            return 1;
        default:
            abort();
        }
    }

    // Will default to interpret for now
    FILE* file = fopen(filename, "r");
    if (!file) exit(1);
    if (!interpret_program(file)) {
        if (interpret) {
            execute_program();
        } else {
            FILE* outfile = fopen(out_filename, "w");
            if (!outfile) exit(1);
            compile_program(outfile);
            fclose(outfile);
            // TODO: Make these not static filenames
            system("nasm -g -F dwarf -f elf64 -o out.o out.asm");
            system("ld -o out out.o");
        }
    }

    fclose(file);

    return EXIT_SUCCESS;
}
