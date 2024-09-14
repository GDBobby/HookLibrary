#include "HookMapping.h"

#include <functional>

namespace HookLibrary {
	Hook* Redirector::hooks[MAX_HOOK_COUNT]{};

	template<size_t Index>
	const void HookRedirection() {
		//printf("template test:%zu\n", Index);
		Redirector::Hook_Simple_Func(Redirector::hooks[Index]);
	}
	/* 
	* additional templates can be made to pass an arbitrary amount of parameters
	
	template<size_t Index>
	const void HookRedirectionPassParameters() {

		Redirector::Hook_Context_Func(Redirector::hooks[Index], parameters[Index]...);
	}
	*/

	void (*Redirector::Hook_Simple_Func)(Hook*);

	template <std::size_t... Is>
	constexpr auto make_simple_array_helper(std::integer_sequence<std::size_t, Is...>)
	{
		return std::array{ HookRedirection<Is>... };
	}

	template<std::size_t N>
	constexpr auto make_simple_array() {
		return make_simple_array_helper(std::make_index_sequence<N>{});
	}


	const std::array<const void(*)(), MAX_HOOK_COUNT> Redirector::wrapperSimpleFuncs = make_simple_array<MAX_HOOK_COUNT>();

	namespace Manager {
		Hook* AddHook(void* source, uint32_t flags) {
			for (size_t i = 0; i < MAX_HOOK_COUNT; i++) {
				if (Redirector::hooks[i] != nullptr) {
					Redirector::hooks[i] = Hook::Create(source, Redirector::wrapperSimpleFuncs[i], flags);
					
					return Redirector::hooks[i];
				}
			}
			assert(false && "failed to find a slot for this hook, raise the value for HOOK_COUNT");
			return nullptr; //warning silencing
		}
		Hook* AddHook(void* source, Hook::Flags flags) {
			return AddHook(source, static_cast<uint32_t>(flags));
		}
		Hook* AddHook(size_t source, uint32_t flags) {
			return AddHook(reinterpret_cast<void*>(source), flags);
		}

		Hook* AddHook(size_t source, Hook::Flags flags) {
			return AddHook(reinterpret_cast<void*>(source), static_cast<uint32_t>(flags));
		}
		Hook* AddHook(void* source) {
			return AddHook(source, 0);
		}
		Hook* AddHook(size_t source) {
			return AddHook(reinterpret_cast<void*>(source), 0);
		}

		void RemoveHookBySource(void* source) {
			for (size_t i = 0; i < MAX_HOOK_COUNT; i++) {
				if (Redirector::hooks[i] != nullptr) {
					if (Redirector::hooks[i]->GetSource() == source) {
						delete Redirector::hooks[i];
						return;
					}
				}
			}
			assert(false && "failed to remove by source");
		}
		void RemoveHookByHook(void* hook) {
			assert(hook != nullptr);

			for (size_t i = 0; i < MAX_HOOK_COUNT; i++) {
				if (Redirector::hooks[i] == hook) {
					delete Redirector::hooks[i];
					return;
				}
			}
			assert(false && "failed to remove by hook");
		}
		void Cleanup() {
			for (size_t i = 0; i < MAX_HOOK_COUNT; i++) {
				if (Redirector::hooks[i] != nullptr) {
					delete Redirector::hooks[i];
				}
			}
		}
	}

}//namespace HookLibrary