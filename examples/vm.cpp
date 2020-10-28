#include <assert.h>

#include "combinator.cpp"
#include "binary.cpp"

#define ALLOCATOR(name) void * name(size_t size)
typedef ALLOCATOR(Alloc);

#define allocs(name) (name *)alloc(sizeof(name))

ALLOCATOR(alloc_default) {
    printf("%zd bytes speicher reserviert\n", size);

    void *result = malloc(size);

    return result;
}

Alloc *alloc = alloc_default;

enum Op {
    OP_MOV_IMM_REG = 0x10,
    OP_MOV_REG_REG = 0x11,
    OP_MOV_REG_MEM = 0x12,
    OP_MOV_MEM_REG = 0x13,

    OP_ADD_REG_REG = 0x14,

    OP_JMP_NEQ     = 0x15,

    OP_COUNT,
};

enum Reg {
    REG_ACC,

    REG_R1,
    REG_R2,
    REG_R3,
    REG_R4,
    REG_R5,
    REG_R6,
    REG_R7,
    REG_R8,

    REG_COUNT,
};

struct Cpu {
    uint8_t  * mem;
    uint64_t   size;

    uint8_t    pc; // program counter
    uint16_t   sp; // stack pointer
    uint16_t   fp; // frame pointer
    uint16_t   regs[REG_COUNT];
};

uint16_t
switch_bytes16(uint16_t val) {
    uint16_t result = ((val & 0xFF00) >> 8) | ((val & 0x00FF) << 8);

    return result;
}

uint16_t
reg_read(Cpu *cpu, uint8_t reg) {
    assert(reg < REG_COUNT);

    uint16_t result = cpu->regs[reg];

    return result;
}

void
reg_write(Cpu *cpu, uint8_t reg, uint16_t val) {
    assert(reg < REG_COUNT);

    cpu->regs[reg] = val;
}

uint8_t *
mem_create(size_t size) {
    uint8_t *result = (uint8_t *)alloc(size);

    for ( int i = 0; i < size; ++i ) {
        result[i] = 0;
    }

    return result;
}

uint8_t
mem_read(Cpu *cpu, uint16_t addr) {
    uint8_t result = *(cpu->mem + addr);

    return result;
}

uint16_t
mem_read16(Cpu *cpu, uint16_t addr) {
    uint16_t result = *(uint16_t *)(cpu->mem + addr);

    return result;
}

void
mem_write(Cpu *cpu, uint16_t addr, uint8_t val) {
    *(cpu->mem + addr) = val;
}

void
mem_write16(Cpu *cpu, uint16_t addr, uint16_t val) {
    *(uint16_t *)(cpu->mem + addr) = val;
}

void
mem_print(Cpu *cpu, uint16_t addr, char *label) {
    printf("\n======================== %s =============================\n", label);
    printf("MEM 0x%04x: ", addr);
    for ( uint16_t i = 0; i < 8; ++i ) {
        auto val = mem_read(cpu, addr+i);
        printf("0x%02x ", val);
    }
    printf("\n==========================================================\n");
}

uint8_t
instr_fetch(Cpu *cpu) {
    uint8_t instr = *(cpu->mem + cpu->pc);
    cpu->pc += 1;

    return instr;
}

uint16_t
instr_fetch16(Cpu *cpu) {
    uint16_t instr = *(uint16_t *)(cpu->mem + cpu->pc);
    cpu->pc += 2;

    return instr;
}

void
instr_exec(Cpu *cpu, uint16_t instr) {
    switch ( instr ) {
        case OP_MOV_IMM_REG: {
            uint16_t imm = instr_fetch16(cpu);
            uint8_t reg = instr_fetch(cpu);

            reg_write(cpu, reg, imm);
        } break;

        case OP_MOV_REG_REG: {
            auto src = instr_fetch(cpu);
            auto dst = instr_fetch(cpu);
            uint16_t val = reg_read(cpu, src);

            reg_write(cpu, dst, val);
        } break;

        case OP_MOV_REG_MEM: {
            auto reg  = instr_fetch(cpu);
            auto addr = instr_fetch16(cpu);
            uint16_t val = reg_read(cpu, reg);

            mem_write16(cpu, addr, val);
        } break;

        case OP_MOV_MEM_REG: {
            auto addr = instr_fetch16(cpu);
            auto reg  = instr_fetch(cpu);
            uint16_t val = mem_read16(cpu, addr);

            reg_write(cpu, reg, val);
        } break;

        case OP_ADD_REG_REG: {
            auto r1 = instr_fetch(cpu);
            auto r2 = instr_fetch(cpu);

            auto imm1 = reg_read(cpu, r1);
            auto imm2 = reg_read(cpu, r2);

            reg_write(cpu, REG_ACC, imm1+imm2);
        } break;

        case OP_JMP_NEQ: {
            auto imm = instr_fetch16(cpu);
            auto addr = instr_fetch16(cpu);
            auto acc = reg_read(cpu, REG_ACC);

            if ( imm != acc ) {
                cpu->pc = (uint8_t)addr;
            }
        } break;
    }
}

void
cpu_print(Cpu *cpu) {
    printf("[PC] = 0x%04x, [ACC] = 0x%04x\n", cpu->pc, cpu->regs[REG_ACC]);
    for ( int i = REG_R1; i < REG_COUNT; ++i ) {
        printf("[REG%d] = 0x%04x\n", i, cpu->regs[i]);
    }
}

void
cpu_step(Cpu *cpu) {
    auto instr = instr_fetch(cpu);
    instr_exec(cpu, instr);
}

Cpu *
cpu_create(void *mem, size_t size) {
    Cpu *result = allocs(Cpu);

    *result = {};

    result->mem  = (uint8_t *)mem;
    result->size = size;

    for ( int i = 0; i < REG_COUNT; ++i ) {
        result->regs[i] = 0;
    }

    return result;
}

void debug_print(Cpu *cpu) {
    printf("\n======================= REGS =============================\n");
    cpu_print(cpu);
    printf("\n==========================================================\n");
}

int main(int argc, char const* argv[]) {
    int mem_size = 256*256;

    uint8_t *mem = mem_create(mem_size);
    Cpu *cpu = cpu_create(mem, mem_size);

    uint32_t pc = 0;
    uint8_t start = 0;

    mem[pc++] = OP_MOV_MEM_REG;
    mem[pc++] = 0x00;
    mem[pc++] = 0x01;
    mem[pc++] = REG_R1;

    mem[pc++] = OP_MOV_IMM_REG;
    mem[pc++] = 0x01;
    mem[pc++] = 0x00;
    mem[pc++] = REG_R2;

    mem[pc++] = OP_ADD_REG_REG;
    mem[pc++] = REG_R1;
    mem[pc++] = REG_R2;

    mem[pc++] = OP_MOV_REG_MEM;
    mem[pc++] = REG_ACC;
    mem[pc++] = 0x00;
    mem[pc++] = 0x01;

    mem[pc++] = OP_JMP_NEQ;
    mem[pc++] = 0x03;
    mem[pc++] = 0x00;
    mem[pc++] = start;

    debug_print(cpu);
    mem_print(cpu, cpu->pc, "PC");
    mem_print(cpu, 0x0100, "MEM");

    for ( ;; ) {
        getchar();

        cpu_step(cpu);
        debug_print(cpu);
        mem_print(cpu, cpu->pc, "PC");
        mem_print(cpu, 0x0100, "MEM");
    }

    return 0;
}
