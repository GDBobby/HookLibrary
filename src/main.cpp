#include <cstdio>
#include "Hook.h"

#include <type_traits>
#include <functional>
#include <cassert>
#include <string>
#include <vector>

#include <random>

#include "HookMapping.h"

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

std::vector<uint16_t> randomValues{};


void FuncWithTrampoline() {
	printf("trampoline\t");
	assert(Foo_Tr != nullptr && "failed to init trampoline");
	Foo_Tr();
}

void FuncBlock() {
	printf("block\n");
}

void HookSimpleController(HookLibrary::Hook* hook) {
	printf("hook controller : %zu\n", reinterpret_cast<size_t>(hook));
	if (hook->GetFlags() & HookLibrary::Hook::Trampoline) {
		reinterpret_cast<void(*)()>(hook->GetTrampoline())();
	}
}

std::vector<HookLibrary::Hook*> hookContainer{};

int main() {
	Foo();

	HookLibrary::Redirector::Hook_Simple_Func = HookSimpleController;

	{ //blocking hook
		HookLibrary::Hook hook{ reinterpret_cast<void*>(&Foo), reinterpret_cast<void*>(&FuncBlock), 0};
		Foo();
	}
	Foo();
	{
		void* srcPtr = reinterpret_cast<void*>(&Foo);
		void* dstPtr = reinterpret_cast<void*>(HookLibrary::Redirector::wrapperSimpleFuncs[0]);
		uint32_t flags = HookLibrary::Hook::Flags::Trampoline;
		HookLibrary::Hook hook{
			srcPtr, 
			dstPtr, 
			flags
		};
		HookLibrary::Redirector::hooks[0] = &hook;
		printf("before template capture : %zu\n", &hook);
		Foo();
		printf("after template capture\n");
	}
	{
		HookLibrary::Manager::AddHook(reinterpret_cast<void*>(&Foo), HookLibrary::Hook::Flags::Trampoline);

	}
	Foo();

	//std::random_device ranDev{};
	//std::default_random_engine rEng{ranDev};

	//const uint16_t rangeHigh = 200;
	//const uint16_t rangeLow = 0;

	//std::uniform_int_distribution<std::mt19937::result_type> dist(1, 200);

	//for (uint16_t i = 0; i < 100; i++) {
	//	randomValues.push_back(dist(rEng));
	//}


	//{
	//	HookLibrary::Subhook hook{ reinterpret_cast<void*>(&FooString), reinterpret_cast<void*>(&FuncWithTrampolineTemplate<void(*)(std::string)>), HookLibrary::Subhook::Flags::Trampoline | HookLibrary::Subhook::Flags::Offset_64_Bit | HookLibrary::Subhook::Flags::AllocNearby };
	//	std::string inputString = "hello world[string]";
	//	FuncWithTrampolineTemplate(reinterpret_cast<void(*)(std::string)>(hook.GetTrampoline()), inputString);
	//}

	return 0;
}