#pragma once

#include "Hook.h"
#include <Windows.h>

#include <array>
#include <cassert>

namespace HookLibrary {

	constexpr size_t MAX_HOOK_COUNT = 256;

	struct Redirector {
		static Hook* hooks[MAX_HOOK_COUNT]; //start off as null

		//static const void(*wrapperFuncs[HOOK_COUNT])();
		static const std::array<const void(*)(), MAX_HOOK_COUNT> wrapperSimpleFuncs;

		static void(*Hook_Simple_Func)(Hook*);
	};

	namespace Manager {

		void Cleanup();

		Hook* AddHook(void* source, uint32_t flags);
		Hook* AddHook(size_t source, uint32_t flags);
		Hook* AddHook(void* source, Hook::Flags flags);
		Hook* AddHook(size_t source, Hook::Flags flags);
		Hook* AddHook(void* source);
		Hook* AddHook(size_t source);

		void RemoveHookBySource(void* source);
		void RemoveHookByHook(void* hook);
	};
}