#include "Hook.h"

#include <cassert>
#include <string.h>
#include <cmath>

static uint8_t prefixes[] = {
  0xF0, 0xF2, 0xF3,
  0x2E, 0x36, 0x3E, 0x26, 0x64, 0x65,
  0x66, /* operand size override */
  0x67  /* address size override */
};

struct opcode_info {
    enum ID : uint8_t {
        ADD_AL_imm8,
        ADD_EAX_imm32,
        ADD_rm8_imm8,
        ADD_rm32_imm32,
        ADD_rm32_imm8,
        ADD_rm8_r8,
        ADD_rm32_r32,
        ADD_r8_rm8,
        ADD_r32_rm32,
        AND_AL_imm8,
        AND_EAX_imm32,
        AND_rm8_imm8,
        AND_rm32_imm32,
        AND_rm32_imm8,
        AND_rm8_r8,
        AND_rm32_r32,
        AND_r8_rm8,
        AND_r32_rm32,
        CALL_rel32,
        CALL_rm32,
        CMP_rm32_imm8,
        CMP_rm32_r32,
        CMP_imm16_32,
        DEC_rm32,
        DEC_r32,
        ENTER_imm16_imm8,
        FLD_m32fp,
        FLD_m64fp,
        FLD_m80fp,
        INT_3,
        JMP_rel32,
        JMP_rm32,
        LEA_r32, m,
        LEAVE,
        MOV_rm8, r8,
        MOV_rm32_r32,
        MOV_r8_rm8,
        MOV_r32r_m32,
        MOV_rm16_Sreg,
        MOV_Sreg_rm16,
        MOV_AL_moffs8,
        MOV_EAX_moffs32,
        MOV_moffs8_AL,
        MOV_moffs32_EAX,
        MOV_r8_imm8,
        MOV_r32_imm32,
        MOV_rm8_imm8,
        MOV_rm32_imm32,
        NOP,
        OR_AL_imm8,
        OR_EAX_imm32,
        OR_rm8_imm8,
        OR_rm32_imm32,
        OR_rm32_imm8,
        OR_rm8_r8,
        OR_rm32_r32,
        OR_r8_rm8,
        OR_r32_rm32,
        POP_rm32,
        POP_r32,
        PUSH_rm32,
        PUSH_r32,
        PUSH_imm8,
        PUSH_imm32,
        rET,
        rET_imm16,
        SUB_AL_imm8,
        SUB_EAX_imm32,
        SUB_rm8_imm8,
        SUB_rm32_imm32,
        SUB_rm32_imm8,
        SUB_rm8_r8,
        SUB_rm32_r32,
        SUB_r8_rm8,
        SUB_r32_rm32,
        TEST_AL_imm8,
        TEST_EAX_imm32,
        TEST_rm8_imm8,
        TEST_rm32_imm32,
        TEST_rm8_r8,
        TEST_rm32_r32,
        XOR_AL_imm8,
        XOR_EAX_imm32,
        XOR_rm8_imm8,
        XOR_rm32_imm32,
        XOR_rm32_imm8,
        XOR_rm8_r8,
        XOR_rm32_r32,
        XOR_r8_rm8,
        XOR_r32_rm32
    };
    uint8_t opcode;
    uint8_t reg_opcode;
    uint32_t flags;

    opcode_info(uint8_t opcode, uint8_t reg_opcode, uint32_t flags) : opcode{ opcode }, reg_opcode{ reg_opcode }, flags{ flags } {}
};


namespace HookLibrary {
	int Hook::FirstInstructionDisassembly(void* source, int* reloc_op_offset) {
		enum flags {
			MODRM = 1,
			PLUS_R = 1 << 1,
			REG_OPCODE = 1 << 2,
			IMM8 = 1 << 3,
			IMM16 = 1 << 4,
			IMM32 = 1 << 5,
			RELOC = 1 << 6
		};
		static uint8_t prefixes[] = {
		  0xF0, 0xF2, 0xF3,
		  0x2E, 0x36, 0x3E, 0x26, 0x64, 0x65,
		  0x66, /* operand size override */
		  0x67  /* address size override */
		};
        const opcode_info opcodes[] = {
            /* ADD AL, imm8      */ {0x04, 0, IMM8},
            /* ADD EAX, imm32    */ {0x05, 0, IMM32},
            /* ADD r/m8, imm8    */ {0x80, 0, MODRM | REG_OPCODE | IMM8},
            /* ADD r/m32, imm32  */ {0x81, 0, MODRM | REG_OPCODE | IMM32},
            /* ADD r/m32, imm8   */ {0x83, 0, MODRM | REG_OPCODE | IMM8},
            /* ADD r/m8, r8      */ {0x00, 0, MODRM},
            /* ADD r/m32, r32    */ {0x01, 0, MODRM},
            /* ADD r8, r/m8      */ {0x02, 0, MODRM},
            /* ADD r32, r/m32    */ {0x03, 0, MODRM},
            /* AND AL, imm8      */ {0x24, 0, IMM8},
            /* AND EAX, imm32    */ {0x25, 0, IMM32},
            /* AND r/m8, imm8    */ {0x80, 4, MODRM | REG_OPCODE | IMM8},
            /* AND r/m32, imm32  */ {0x81, 4, MODRM | REG_OPCODE | IMM32},
            /* AND r/m32, imm8   */ {0x83, 4, MODRM | REG_OPCODE | IMM8},
            /* AND r/m8, r8      */ {0x20, 0, MODRM},
            /* AND r/m32, r32    */ {0x21, 0, MODRM},
            /* AND r8, r/m8      */ {0x22, 0, MODRM},
            /* AND r32, r/m32    */ {0x23, 0, MODRM},
            /* CALL rel32        */ {0xE8, 0, IMM32 | RELOC},
            /* CALL r/m32        */ {0xFF, 2, MODRM | REG_OPCODE},
                /* CMP r/m32, imm8   */{ 0x83, 7, MODRM | REG_OPCODE | IMM8 },
                /* CMP r/m32, r32    */{ 0x39, 0, MODRM },
                /* CMP imm16/32      */{ 0x3D, 0, IMM32 },
                /* DEC r/m32         */{ 0xFF, 1, MODRM | REG_OPCODE },
                /* DEC r32           */{ 0x48, 0, PLUS_R },
                /* ENTER imm16, imm8 */{ 0xC8, 0, IMM16 | IMM8 },
                /* FLD m32fp         */{ 0xD9, 0, MODRM | REG_OPCODE },
                /* FLD m64fp         */{ 0xDD, 0, MODRM | REG_OPCODE },
                /* FLD m80fp         */{ 0xDB, 5, MODRM | REG_OPCODE },
                /* INT 3             */{ 0xCC, 0, 0 },
                /* JMP rel32         */{ 0xE9, 0, IMM32 | RELOC },
                /* JMP r/m32         */{ 0xFF, 4, MODRM | REG_OPCODE },
                /* LEA r32,m         */{ 0x8D, 0, MODRM },
                /* LEAVE             */{ 0xC9, 0, 0 },
                /* MOV r/m8,r8       */{ 0x88, 0, MODRM },
                /* MOV r/m32,r32     */{ 0x89, 0, MODRM },
                /* MOV r8,r/m8       */{ 0x8A, 0, MODRM },
                /* MOV r32,r/m32     */{ 0x8B, 0, MODRM },
                /* MOV r/m16,Sreg    */{ 0x8C, 0, MODRM },
                /* MOV Sreg,r/m16    */{ 0x8E, 0, MODRM },
                /* MOV AL,moffs8     */{ 0xA0, 0, IMM8 },
                /* MOV EAX,moffs32   */{ 0xA1, 0, IMM32 },
                /* MOV moffs8,AL     */{ 0xA2, 0, IMM8 },
                /* MOV moffs32,EAX   */{ 0xA3, 0, IMM32 },
                /* MOV r8, imm8      */{ 0xB0, 0, PLUS_R | IMM8 },
                /* MOV r32, imm32    */{ 0xB8, 0, PLUS_R | IMM32 },
                /* MOV r/m8, imm8    */{ 0xC6, 0, MODRM | REG_OPCODE | IMM8 },
                /* MOV r/m32, imm32  */{ 0xC7, 0, MODRM | REG_OPCODE | IMM32 },
                /* NOP               */{ 0x90, 0, 0 },
                /* OR AL, imm8       */{ 0x0C, 0, IMM8 },
                /* OR EAX, imm32     */{ 0x0D, 0, IMM32 },
                /* OR r/m8, imm8     */{ 0x80, 1, MODRM | REG_OPCODE | IMM8 },
                /* OR r/m32, imm32   */{ 0x81, 1, MODRM | REG_OPCODE | IMM32 },
                /* OR r/m32, imm8    */{ 0x83, 1, MODRM | REG_OPCODE | IMM8 },
                /* OR r/m8, r8       */{ 0x08, 0, MODRM },
                /* OR r/m32, r32     */{ 0x09, 0, MODRM },
                /* OR r8, r/m8       */{ 0x0A, 0, MODRM },
                /* OR r32, r/m32     */{ 0x0B, 0, MODRM },
                /* POP r/m32         */{ 0x8F, 0, MODRM | REG_OPCODE },
                /* POP r32           */{ 0x58, 0, PLUS_R },
                /* PUSH r/m32        */{ 0xFF, 6, MODRM | REG_OPCODE },
                /* PUSH r32          */{ 0x50, 0, PLUS_R },
                /* PUSH imm8         */{ 0x6A, 0, IMM8 },
                /* PUSH imm32        */{ 0x68, 0, IMM32 },
                /* RET               */{ 0xC3, 0, 0 },
                /* RET imm16         */{ 0xC2, 0, IMM16 },
                /* SUB AL, imm8      */{ 0x2C, 0, IMM8 },
                /* SUB EAX, imm32    */{ 0x2D, 0, IMM32 },
                /* SUB r/m8, imm8    */{ 0x80, 5, MODRM | REG_OPCODE | IMM8 },
                /* SUB r/m32, imm32  */{ 0x81, 5, MODRM | REG_OPCODE | IMM32 },
                /* SUB r/m32, imm8   */{ 0x83, 5, MODRM | REG_OPCODE | IMM8 },
                /* SUB r/m8, r8      */{ 0x28, 0, MODRM },
                /* SUB r/m32, r32    */{ 0x29, 0, MODRM },
                /* SUB r8, r/m8      */{ 0x2A, 0, MODRM },
                /* SUB r32, r/m32    */{ 0x2B, 0, MODRM },
                /* TEST AL, imm8     */{ 0xA8, 0, IMM8 },
                /* TEST EAX, imm32   */{ 0xA9, 0, IMM32 },
                /* TEST r/m8, imm8   */{ 0xF6, 0, MODRM | REG_OPCODE | IMM8 },
                /* TEST r/m32, imm32 */{ 0xF7, 0, MODRM | REG_OPCODE | IMM32 },
                /* TEST r/m8, r8     */{ 0x84, 0, MODRM },
                /* TEST r/m32, r32   */{ 0x85, 0, MODRM },
                /* XOR AL, imm8      */{ 0x34, 0, IMM8 },
                /* XOR EAX, imm32    */{ 0x35, 0, IMM32 },
                /* XOR r/m8, imm8    */{ 0x80, 6, MODRM | REG_OPCODE | IMM8 },
                /* XOR r/m32, imm32  */{ 0x81, 6, MODRM | REG_OPCODE | IMM32 },
                /* XOR r/m32, imm8   */{ 0x83, 6, MODRM | REG_OPCODE | IMM8 },
                /* XOR r/m8, r8      */{ 0x30, 0, MODRM },
                /* XOR r/m32, r32    */{ 0x31, 0, MODRM },
                /* XOR r8, r/m8      */{ 0x32, 0, MODRM },
                /* XOR r32, r/m32    */{ 0x33, 0, MODRM }
        };

        uint8_t* code = reinterpret_cast<uint8_t*>(source);
        size_t i;
        int length = 0;
        int operand_size = 4;
        uint8_t opcode = 0;
        int found_opcode = false;

        for (i = 0; i < sizeof(prefixes) / sizeof(*prefixes); i++) {
            if (code[length] == prefixes[i]) {
                length++;
                if (prefixes[i] == 0x66) {
                    operand_size = 2;
                }
            }
        }
#ifdef SUBHOOK_X86_64
        if ((code[length] & 0xF0) == 0x40) {
            //This is a REX prefix (40H - 4FH). REX prefixes are valid only in 64-bit mode
            uint8_t rex = code[length++];
            if (rex & 8) {
                operand_size = 8;
            }
        }
#endif

        for (i = 0; i < sizeof(opcodes) / sizeof(*opcodes); i++) {
            if (code[length] == opcodes[i].opcode) {
                if (opcodes[i].flags & REG_OPCODE){
                    found_opcode = ((code[length + 1] >> 3) & 7) == opcodes[i].reg_opcode;
                }
                else {
                    found_opcode = true;
                }
            }
            if ((opcodes[i].flags & PLUS_R) && (code[length] & 0xF8) == opcodes[i].opcode) {
                found_opcode == true;
            }
            if (found_opcode) {
                opcode = code[length++];
                break;
            }
		}

        if (!found_opcode) {
            return 0;
        }
        if (reloc_op_offset != nullptr && (opcodes[i].flags & RELOC)) {
            *reloc_op_offset = length;
        }
        if (opcodes[i].flags & MODRM) {
            uint8_t modrm = code[length++];
            uint8_t mod = modrm >> 6;
            uint8_t rm = modrm & 0x07;
            if (mod != 3 && rm == 4) {
                uint8_t sib = code[length++];
                uint8_t base = sib & 0x07;
                if (base == 5) {
                    if (mod == 1) {
                        length++;
                    }
                    else {
                        length += 4;
                    }
                }
            }

#ifdef SUBHOOK_X86_64
            if (reloc_op_offset != nullptr && mod == 0 && rm == 5) {
                *reloc_op_offset = static_cast<int32_t>(length);
            }
#endif
            if (mod == 1) {
                length++;
            }
            else if (mod == 2 || (mod == 0 && rm == 5)) {
                length += 4;
            }
        }
        if (opcodes[i].flags & IMM8) {
            length++;
        }
        if (opcodes[i].flags & IMM16) {
            length += 2;
        }
        if (opcodes[i].flags & IMM32) {
            length += operand_size;
        }

		return length;
	}
}//namespace HookLibrary