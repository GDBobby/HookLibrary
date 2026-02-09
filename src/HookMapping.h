#pragma once

#include "Hook.h"
#include <Windows.h>

#include <array>
#include <cassert>

#include <functional>

namespace HookLibrary {

	template<std::size_t Count>
	struct Redirector {
		using HookCount = Count;

		static Hook* hooks[HookCount]; //start off as null

		static const std::array<const void(*)(), HookCount> wrapperSimpleFuncs;

		static std::function<void(Hook*)> Hook_Simple_Func;
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