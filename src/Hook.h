#pragma once
#include <cstdint>
#include <cstdio>

#if defined _M_IX86 || defined __i386__ //i cant test this, i have an amd
#define SUBHOOK_X86
#define SUBHOOK_BITS 32
#elif defined _M_AMD64 || defined __amd64__
#define SUBHOOK_X86_64
#define SUBHOOK_BITS 64
#else
#error Unsupported architecture
#endif

#if defined _WIN32 || defined __CYGWIN__
#define SUBHOOK_WINDOWS
#elif defined __linux__
#define SUBHOOK_UNIX
#else
#error Unsupported operating system
#endif

#if !defined SUBHOOK_API
#if defined SUBHOOK_X86
#if defined SUBHOOK_WINDOWS
#define SUBHOOK_API __cdecl
#elif defined SUBHOOK_UNIX
#define SUBHOOK_API __attribute__((cdecl))
#endif
#else
#define SUBHOOK_API
#endif
#endif

namespace HookLibrary {
	//struct DisassemblyHandler {
	//private:
	//	enum Flags {
	//		MODRM = 1,
	//		PLUS_R = 1 << 1,
	//		REG_OPCODE = 1 << 2,
	//		IMM8 = 1 << 3,
	//		IMM16 = 1 << 4,
	//		IMM32 = 1 << 5,
	//		RELOC = 1 << 6
	//	};
	//	static const uint8_t prefixes[];

	//public:
	//	void* source;
	//	int* reloc_op_offset;
	//	DisassemblyHandler() : source{ nullptr }, reloc_op_offset{ nullptr } {

	//	}
	//};

	//this is a function pointer??
	typedef int (SUBHOOK_API* DisassemblyHandler)(
		void* src,
		int* reloc_op_offset);

	class Hook {
	public:
		enum Flags : uint8_t {

			Trampoline = 1,
			PassContext = 2,

		};
	private:
		static DisassemblyHandler disassemblyHandler;

		bool installed{ false };
		void* source;
		void* destination;
		Flags flags;
		void* code{ nullptr };
		void* trampoline{ nullptr };
		size_t jmp_size{ 0 };
		size_t trampoline_size{ 0 };
		size_t trampoline_length{ 0 };

		//returns true on failure
		void Unprotect(void* address, size_t size);
		void* AllocCode(void* targetAddress, size_t size);
		void FreeCode(void* address);

		void Make_jmp(void* source, void* destination, Hook::Flags flags);
		bool MakeTrampoline();

	public:
		uint8_t hookID = 0;

		Hook(void* source, void* destination, uint32_t flags);
		~Hook();

		static void SetDisassemblyHandler(DisassemblyHandler disasmHandler);
		void* GetSource();
		void* GetDestination();
		void* GetTrampoline();

		void Install();
		bool IsInstalled();
		void Uninstall();
		void Free();
		Flags GetFlags() const { return flags; }

		void* ReadDestination();
		//defined in subhookx86.cpp
		static int FirstInstructionDisassembly(void* source, int* reloc_op_offset);
		// end defined in subhookx86.cpp

		static Hook* Create(void* source, void* destination, Flags flags) {

			return new Hook(source, destination, flags);
		}
		static Hook* Create(void* source, void* destination, uint32_t flags) {
			return new Hook(source, destination, flags);
		}
	};
}

