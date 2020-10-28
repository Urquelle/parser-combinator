#include <assert.h>

#include "combinator.cpp"
#include "binary.cpp"

#define ALLOCATOR(name) void * name(size_t size)
typedef ALLOCATOR(Alloc);

#define allocs(name) (name *)alloc(sizeof(name))

#define BYTES(n) n

ALLOCATOR(alloc_default) {
    printf("%zd bytes speicher reserviert\n", size);

    void *result = malloc(size);

    return result;
}

Alloc *alloc = alloc_default;

enum Op {
    OP_MOV_IMM_REG = 0x10,
    OP_MOV_REG_REG,
    OP_MOV_REG_MEM,
    OP_MOV_MEM_REG,

    OP_ADD_REG_REG,

    OP_JMP_NEQ,

    OP_PSH_IMM,
    OP_PSH_REG,
    OP_POP,

    OP_CAL_IMM,
    OP_CAL_REG,

    OP_RET,

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
    size_t     size;

    uint16_t   pc; // program counter
    size_t     sp; // stack pointer
    size_t     fp; // frame pointer
    uint16_t   stack_frame_size;
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
mem_read(uint8_t *mem, size_t addr) {
    uint8_t result = *(mem + addr);

    return result;
}

uint16_t
mem_read16(uint8_t *mem, size_t addr) {
    uint16_t result = *(uint16_t *)(mem + addr);

    return result;
}

uint8_t
mem_write(uint8_t *mem, size_t addr, uint8_t val) {
    *(mem + addr) = val;

    return BYTES(1);
}

uint8_t
mem_write16(uint8_t *mem, size_t addr, uint16_t val) {
    *(uint16_t *)(mem + addr) = val;

    return BYTES(2);
}

void
mem_print(Cpu *cpu, size_t addr, char *label, size_t n = 8) {
    printf("\n======================== %s =============================\n", label);
    printf("MEM 0x%04zx: ", addr);
    for ( uint16_t i = 0; i < n; ++i ) {
        auto val = mem_read(cpu->mem, addr+i);
        printf("0x%02x ", val);
    }
    printf("\n==========================================================\n");
}

uint8_t
fetch(Cpu *cpu) {
    uint8_t instr = *(cpu->mem + cpu->pc);
    cpu->pc += BYTES(1);

    return instr;
}

uint16_t
fetch16(Cpu *cpu) {
    uint16_t instr = *(uint16_t *)(cpu->mem + cpu->pc);
    cpu->pc += BYTES(2);

    return instr;
}

void
push(Cpu *cpu, uint16_t val) {
    mem_write16(cpu->mem, cpu->sp, val);
    cpu->sp -= BYTES(2);
    cpu->stack_frame_size += 2;
}

uint16_t
pop(Cpu *cpu) {
    cpu->sp += 2;
    uint16_t result = mem_read16(cpu->mem, cpu->sp);
    cpu->stack_frame_size -= 2;

    return result;
}

void
push_state(Cpu *cpu) {
    push(cpu, reg_read(cpu, REG_R1));
    push(cpu, reg_read(cpu, REG_R2));
    push(cpu, reg_read(cpu, REG_R3));
    push(cpu, reg_read(cpu, REG_R4));
    push(cpu, reg_read(cpu, REG_R5));
    push(cpu, reg_read(cpu, REG_R6));
    push(cpu, reg_read(cpu, REG_R7));
    push(cpu, reg_read(cpu, REG_R8));
    push(cpu, cpu->pc);
    push(cpu, cpu->stack_frame_size+2);

    cpu->fp = cpu->sp;
    cpu->stack_frame_size = 0;
}

void
pop_state(Cpu *cpu) {
    cpu->sp = cpu->fp;
    cpu->stack_frame_size = pop(cpu);
    auto stack_frame_size = cpu->stack_frame_size;

    cpu->pc = pop(cpu);
    reg_write(cpu, REG_R8, pop(cpu));
    reg_write(cpu, REG_R7, pop(cpu));
    reg_write(cpu, REG_R6, pop(cpu));
    reg_write(cpu, REG_R5, pop(cpu));
    reg_write(cpu, REG_R4, pop(cpu));
    reg_write(cpu, REG_R3, pop(cpu));
    reg_write(cpu, REG_R2, pop(cpu));
    reg_write(cpu, REG_R1, pop(cpu));

    auto num_args = pop(cpu);
    for ( int i = 0; i < num_args; ++i ) {
        pop(cpu);
    }

    cpu->fp = cpu->fp + stack_frame_size;
}

void
exec(Cpu *cpu, uint16_t instr) {
    switch ( instr ) {
        case OP_MOV_IMM_REG: {
            uint16_t imm = fetch16(cpu);
            uint8_t reg = fetch(cpu);

            reg_write(cpu, reg, imm);
        } break;

        case OP_MOV_REG_REG: {
            auto src = fetch(cpu);
            auto dst = fetch(cpu);
            uint16_t val = reg_read(cpu, src);

            reg_write(cpu, dst, val);
        } break;

        case OP_MOV_REG_MEM: {
            auto reg  = fetch(cpu);
            auto addr = fetch16(cpu);
            uint16_t val = reg_read(cpu, reg);

            mem_write16(cpu->mem, addr, val);
        } break;

        case OP_MOV_MEM_REG: {
            auto addr = fetch16(cpu);
            auto reg  = fetch(cpu);
            uint16_t val = mem_read16(cpu->mem, addr);

            reg_write(cpu, reg, val);
        } break;

        case OP_ADD_REG_REG: {
            auto r1 = fetch(cpu);
            auto r2 = fetch(cpu);

            auto imm1 = reg_read(cpu, r1);
            auto imm2 = reg_read(cpu, r2);

            reg_write(cpu, REG_ACC, imm1+imm2);
        } break;

        case OP_JMP_NEQ: {
            auto imm = fetch16(cpu);
            auto addr = fetch16(cpu);
            auto acc = reg_read(cpu, REG_ACC);

            if ( imm != acc ) {
                cpu->pc = (uint8_t)addr;
            }
        } break;

        case OP_PSH_IMM: {
            auto val = fetch16(cpu);
            push(cpu, val);
        } break;

        case OP_PSH_REG: {
            auto reg = fetch(cpu);
            auto val = reg_read(cpu, reg);
            push(cpu, val);
        } break;

        case OP_POP: {
            auto reg = fetch(cpu);
            auto val = pop(cpu);
            reg_write(cpu, reg, val);
        } break;

        case OP_CAL_IMM: {
            auto addr = fetch16(cpu);
            push_state(cpu);
            cpu->pc = addr;
        } break;

        case OP_CAL_REG: {
            auto reg = fetch(cpu);
            auto addr = reg_read(cpu, reg);
            push_state(cpu);
            cpu->pc = addr;
        } break;

        case OP_RET: {
            pop_state(cpu);
        } break;
    }
}

void
step(Cpu *cpu) {
    auto instr = fetch(cpu);
    exec(cpu, instr);
}

void
cpu_print(Cpu *cpu) {
    printf("[PC] = 0x%04x, [ACC] = 0x%04x\n", cpu->pc, cpu->regs[REG_ACC]);
    for ( int i = REG_R1; i < REG_COUNT; ++i ) {
        printf("[REG%d] = 0x%04x\n", i, cpu->regs[i]);
    }
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

    result->sp = result->size - 2;
    result->fp = result->sp;

    return result;
}

void debug_print(Cpu *cpu) {
    printf("\n======================= REGS =============================\n");
    cpu_print(cpu);
    printf("\n==========================================================\n");
}

int main(int argc, char const* argv[]) {
    size_t mem_size = 256*256;

    uint8_t *mem = mem_create(mem_size);
    Cpu *cpu = cpu_create(mem, mem_size);

    uint16_t pc = 0;
    uint16_t subroutine_address = 0x3000;

    pc += mem_write(mem, pc, OP_PSH_IMM);
    pc += mem_write16(mem, pc, 0x3333);

    pc += mem_write(mem, pc, OP_PSH_IMM);
    pc += mem_write16(mem, pc, 0x2222);

    pc += mem_write(mem, pc, OP_PSH_IMM);
    pc += mem_write16(mem, pc, 0x1111);

    pc += mem_write(mem, pc, OP_MOV_IMM_REG);
    pc += mem_write16(mem, pc, 0x1234);
    pc += mem_write(mem, pc, REG_R1);

    pc += mem_write(mem, pc, OP_MOV_IMM_REG);
    pc += mem_write16(mem, pc, 0x5678);
    pc += mem_write(mem, pc, REG_R4);

    pc += mem_write(mem, pc, OP_PSH_IMM);
    pc += mem_write16(mem, pc, 0x0000);

    pc += mem_write(mem, pc, OP_CAL_IMM);
    pc += mem_write16(mem, pc, subroutine_address);

    pc += mem_write(mem, pc, OP_PSH_IMM);
    pc += mem_write16(mem, pc, 0x4444);

    pc = subroutine_address;

    pc += mem_write(mem, pc, OP_PSH_IMM);
    pc += mem_write16(mem, pc, 0x0102);

    pc += mem_write(mem, pc, OP_PSH_IMM);
    pc += mem_write16(mem, pc, 0x0304);

    pc += mem_write(mem, pc, OP_PSH_IMM);
    pc += mem_write16(mem, pc, 0x0506);

    pc += mem_write(mem, pc, OP_MOV_IMM_REG);
    pc += mem_write16(mem, pc, 0x0708);
    pc += mem_write(mem, pc, REG_R1);

    pc += mem_write(mem, pc, OP_MOV_IMM_REG);
    pc += mem_write16(mem, pc, 0x090A);
    pc += mem_write(mem, pc, REG_R8);

    pc += mem_write(mem, pc, OP_RET);

    debug_print(cpu);
    mem_print(cpu, cpu->pc, "PC");
    mem_print(cpu, cpu->size - 1 - 42, "STACK", 43);

    for ( ;; ) {
        getchar();

        step(cpu);

        debug_print(cpu);
        mem_print(cpu, cpu->pc, "PC");
        mem_print(cpu, cpu->size - 1 - 42, "STACK", 43);
    }

    return 0;
}
