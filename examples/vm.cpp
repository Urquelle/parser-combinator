#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
