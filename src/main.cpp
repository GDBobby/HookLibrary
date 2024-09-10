#include <cstdio>
#include "Hook.h"

#include <type_traits>
#include <functional>
#include <cassert>
#include <string>

void Foo() {
	printf("hello world\n");
}
void FooString(std::string input) {
	printf("%s\n", input.c_str());
}

void (*Foo_Tr)();

//this doesnt work. i think what im envisioning is unreasonable/ not pragmatic
template<typename F, typename... Args>
void FuncWithTrampolineTemplate(F&& f, Args&&... args) {
	printf("trampoline<template>\t");
	std::forward<F>(f)(std::forward<Args>(args)...);
}
//^doesnt work

void FuncWithTrampoline() {
	printf("trampoline\t");
	assert(Foo_Tr != nullptr && "failed to init trampoline");
	Foo_Tr();
}

void FuncBlock() {
	printf("block\n");
}

int main() {
	Foo();

	{ //blocking hook
		HookLibrary::Hook hook{ reinterpret_cast<void*>(&Foo), reinterpret_cast<void*>(&FuncBlock), HookLibrary::Hook::Flags::Offset_64_Bit };
		Foo();
	}
	Foo();
	{ //non-templated trampoline hook
		HookLibrary::Hook hook{ reinterpret_cast<void*>(&Foo), reinterpret_cast<void*>(&FuncWithTrampoline), HookLibrary::Hook::Flags::Trampoline | HookLibrary::Hook::Flags::Offset_64_Bit};
		Foo_Tr = reinterpret_cast<void(*)()>(hook.GetTrampoline());
		Foo();
	}
	Foo();
	{
	}
	//{
	//	HookLibrary::Subhook hook{ reinterpret_cast<void*>(&FooString), reinterpret_cast<void*>(&FuncWithTrampolineTemplate<void(*)(std::string)>), HookLibrary::Subhook::Flags::Trampoline | HookLibrary::Subhook::Flags::Offset_64_Bit | HookLibrary::Subhook::Flags::AllocNearby };
	//	std::string inputString = "hello world[string]";
	//	FuncWithTrampolineTemplate(reinterpret_cast<void(*)(std::string)>(hook.GetTrampoline()), inputString);
	//}

	return 0;
}