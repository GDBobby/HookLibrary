#include "Hook.h"
#include "internal_assem.h"
#include <cassert>

#include <cmath>

bool CheckInt32Overflow(int32_t x) {
	return static_cast<int64_t>(x) < INT32_MIN || static_cast<int64_t>(x) > INT32_MAX;
}
bool CheckInt32Overflow(int64_t x) {
	return x < INT32_MIN || x > INT32_MAX;
}
#define MAX_INSN_LEN 15 /* maximum length of x86 instruction */

namespace HookLibrary {
	DisassemblyHandler Hook::disassemblyHandler{ nullptr };

#ifdef _WIN32
#include <windows.h>

	typedef __int64 QWORD;

	void Hook::Make_jmp(void* source, void* destination, Hook::Flags flags) {
#ifdef SUBHOOK_X86_64
		{
			subhook_jmp64* jmp = reinterpret_cast<subhook_jmp64*>(source);


			jmp->push_opcode = PUSH_OPCODE;
			jmp->push_addr = static_cast<uint32_t>(reinterpret_cast<uint64_t>(destination));
			jmp->mov_opcode = MOV_OPCODE;
			jmp->mov_modrm = JMP64_MOV_MODRM;
			jmp->mov_sib = JMP64_MOV_SIB;
			jmp->mov_offset = JMP64_MOV_OFFSET;
			jmp->mov_addr = static_cast<uint32_t>(reinterpret_cast<size_t>(destination) >> 32);
			jmp->ret_opcode = RET_OPCODE;
			return;
		}
#endif
		subhook_jmp32* jmp = reinterpret_cast<subhook_jmp32*>(source);
		intptr_t src_addr = reinterpret_cast<intptr_t>(source);


		intptr_t dst_addr = reinterpret_cast<intptr_t>(destination);
#ifdef SUBHOOK_X86_64
		int64_t distance = std::abs(src_addr - dst_addr);
		assert(!CheckInt32Overflow(distance) && "overflow");
#endif
		jmp->opcode = JMP_OPCODE;
		jmp->offset = static_cast<int32_t>(dst_addr - (src_addr + sizeof(*jmp)));
	}

	void Hook::Install() {
		assert(!installed && "reinstalling?");
		Make_jmp(source, destination, flags);
		installed = true;
	}
	void Hook::Uninstall() {
		assert(installed && "uninstalling a uninstalled hook");
		memcpy(source, code, jmp_size);
		installed = false;
	}
	void* Hook::ReadDestination() {

#ifdef SUBHOOK_X86_64
		subhook_jmp64* maybe_jmp64 = reinterpret_cast<subhook_jmp64*>(source);

		if (maybe_jmp64->Correct()) {
			return reinterpret_cast<void*>(maybe_jmp64->push_addr & (static_cast<uintptr_t>(maybe_jmp64->mov_addr) << 32));
		}

#else
		subhook_jmp32* maybe_jmp32 = (struct subhook_jmp32*)source;
		if (maybe_jmp32->opcode = JMP_OPCODE) {
			return reinterpret_cast<void*>(maybe_jmp32->offset + reinterpret_cast<uintptr_t>(source) + sizeof(*maybe_jmp32));
		}
#endif
		assert(false && "Failed to read a destination");
		return nullptr;
	}

	void Hook::Unprotect(void* address, size_t size) {
		DWORD old_flags;
		assert(VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &old_flags) != 0 && "failed to virtual protect");
	}
	void* Hook::AllocCode(void* targetAddress, size_t size) {
#if defined _M_AMD64 || defined __amd64__
		if (targetAddress != NULL) {
			SYSTEM_INFO sysInfo;
			DWORD pageSize;
			QWORD offset;

			GetSystemInfo(&sysInfo);
			pageSize = sysInfo.dwPageSize;

			QWORD pivot = reinterpret_cast<QWORD>(targetAddress) & ~(static_cast<QWORD>(pageSize) - 1);

			void* result;
			for (offset = pageSize; offset < (static_cast<QWORD>(1) << 31); offset += pageSize) {
				result = VirtualAlloc(reinterpret_cast<void*>(pivot - offset), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
				if (result != NULL) {
					return result;
				}
				result = VirtualAlloc(reinterpret_cast<void*>(pivot + offset), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
				if (result != NULL) {
					return result;
				}
			}
		}
#endif
		//#else
		void* ret = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		assert(ret != nullptr && "failed VirtualAlloc");
		return ret;

	}
	void Hook::Free() {
		if (trampoline != nullptr) {
			FreeCode(trampoline);
		}
		free(code);
	}

	void Hook::FreeCode(void* address) {
		assert(address != nullptr && "invalid address attempting to be freed");
		assert(VirtualFree(address, 0, MEM_RELEASE) != 0 && "failed to free the code");
		//to get extended error information, use GetLastError
	}

#elif __linux__
	//not currently supported, but not out of the realms of possibility

	void Hook::Install();
	void Hook::Remove();
	void* Hook::ReadDestination();
	int Hook::FirstInstructionDisassembly();

	int Unprotect(void* address, size_t size);
	void* AllocCode(void* targetAddress, size_t size);
	void* FreeCode(void* address, size_t size);

#else
	unsupported, random gibberish to not allow compile
#endif


		size_t GetJmpSize(Hook::Flags flags) {
#ifdef SUBHOOK_X86_64
		return sizeof(subhook_jmp64);
#endif
		return sizeof(subhook_jmp32);
	}

	Hook::Hook(void* source, void* destination, uint32_t flags) :
		source{ source },
		destination{ destination },
		flags{ static_cast<Flags>(flags) },
		jmp_size{ GetJmpSize(static_cast<Hook::Flags>(flags)) },
		trampoline_size{ jmp_size * 2 + MAX_INSN_LEN }
	{
		code = malloc(jmp_size);
		assert(code != nullptr && "failed to malloc code");
		memcpy(code, source, jmp_size);
		Unprotect(source, jmp_size);

		if ((flags & Hook::Flags::Trampoline)) {
			trampoline = AllocCode(nullptr, trampoline_size);
			if (!MakeTrampoline()) {
				printf("failed to make trampoline with AllocCode(nullptr), attempting with AllocCode(source)\n");
				FreeCode(trampoline);
				trampoline = AllocCode(source, trampoline_size);
				assert(MakeTrampoline() && "failed maketrampoline : alloc nearby");
			}
			MakeTrampoline();
		}

		Install();
	}
	Hook::~Hook() {
		Uninstall();
		Free();
	}
	bool Hook::MakeTrampoline() {

		size_t originalSize = 0;
		size_t insn_len;
		intptr_t trampoline_addr = reinterpret_cast<intptr_t>(trampoline);
		intptr_t src_addr = reinterpret_cast<intptr_t>(source);

		//thisl ine needs investigating
		//subhook_disasm_handler_t disasm_handler =
		//subhook_disasm_handler != NULL ? subhook_disasm_handler : subhook_disasm;
		DisassemblyHandler disasm_handler;
		if (disassemblyHandler != nullptr) {
			disasm_handler = disassemblyHandler;
		}
		else {
			disasm_handler = FirstInstructionDisassembly;
		}

		while (originalSize < jmp_size) {
			int reloc_op_offset = 0;
			insn_len = disasm_handler(reinterpret_cast<void*>(src_addr + originalSize), &reloc_op_offset);

			assert(insn_len != 0 && "failed to get instruction length");

			memcpy(reinterpret_cast<void*>(trampoline_addr + originalSize), reinterpret_cast<void*>(src_addr + originalSize), insn_len);

			if (reloc_op_offset > 0) {
				intptr_t offset = trampoline_addr - src_addr;
#ifdef SUBHOOK_X86_64
				//assert(!CheckInt32Overflow(offset) && "overflow error in jmp offset");
				if (CheckInt32Overflow(offset)) {
					printf("overflow error in jmp offset, going to attempt allocating locally\n");
					return false;
				}
#endif
				int32_t* op = reinterpret_cast<int32_t*>(trampoline_addr + originalSize + reloc_op_offset);
				*op -= static_cast<int32_t>(offset);
			}
			originalSize += insn_len;
		}
		trampoline_length = originalSize + jmp_size;
		Make_jmp(reinterpret_cast<void*>(trampoline_addr + originalSize), reinterpret_cast<void*>(src_addr + originalSize), flags);
		return true;
	}

	void* Hook::GetSource() {
		return source;
	}
	void* Hook::GetDestination() {
		return destination;
	}
	void* Hook::GetTrampoline() {
		return trampoline;
	}

	bool Hook::IsInstalled() {
		return installed;
	}

	void Hook::SetDisassemblyHandler(DisassemblyHandler disasmHandler) {
		disassemblyHandler = disasmHandler;
	}

}
