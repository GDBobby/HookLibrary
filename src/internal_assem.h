#pragma once
#include <cstdint>

#define MAX_INSTRUCTION_LENGTH 15
#define JMP_OPCODE 0xE9
#define PUSH_OPCODE 0x68
#define MOV_OPCODE 0xC7
#define RET_OPCODE 0xC3

#define JMP64_MOV_MODRM 0x44 //write to address + 1 byte displacement
#define JMP64_MOV_SIB 0x24 //write to rsp
#define JMP64_MOV_OFFSET 0x04

#pragma pack(push, 1)
struct subhook_jmp32 {
	uint8_t opcode;
	int32_t offset;
};
#define JMP_OPCODE  0xE9
#define PUSH_OPCODE 0x68
#define MOV_OPCODE  0xC7
#define RET_OPCODE  0xC3

#define JMP64_MOV_MODRM  0x44 /* write to address + 1 byte displacement */
#define JMP64_MOV_SIB    0x24 /* write to [rsp] */
#define JMP64_MOV_OFFSET 0x04

struct subhook_jmp64 {
	uint8_t push_opcode;
	uint32_t push_addr; //lower 32bits of addr to jump to
	uint8_t mov_opcode;
	uint8_t mov_modrm;
	uint8_t mov_sib;
	uint8_t mov_offset;
	uint32_t mov_addr;
	uint8_t ret_opcode;

	bool Correct() {
		return push_opcode == PUSH_OPCODE &&
			mov_opcode == MOV_OPCODE &&
			mov_modrm == JMP64_MOV_MODRM &&
			mov_sib == JMP64_MOV_SIB &&
			mov_offset == JMP64_MOV_OFFSET &&
			ret_opcode == RET_OPCODE;
	}
};

#pragma pack(pop)