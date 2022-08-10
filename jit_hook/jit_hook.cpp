#include <cstdint>
#include "offsets/clr_offsets.hpp"
#include "hooks/callback/callback_info.hpp"
#include "hooks/utils.hpp"
#include "hooks/hook.hpp"
#include "hooks/hook_manager.hpp"
#include "hooks/cee_info.hpp"
#include "hooks/profiles/compiler_creator.hpp"

namespace jit_hook
{
#define EXTERNAL extern "C" __declspec(dllexport)

	std::unique_ptr<hooks::hook_manager> manager;

	EXTERNAL uintptr_t get_unboxed_method(const uintptr_t method_descriptor)
	{
		if (!method_descriptor || offsets::is_instantiating_stub(method_descriptor) || !offsets::initialized)
			return method_descriptor;

		const auto unboxed_stub = offsets::get_wrapped_method_descriptor(method_descriptor);
		if (unboxed_stub)
			return unboxed_stub;

		return method_descriptor;
	}

	EXTERNAL uint32_t resolve_token(const uint32_t token)
	{
		const auto handle = hooks::callback::handles_from_token[token];
		return hooks::callback::tokens_from_handle[handle];
	}

	EXTERNAL void compile_method(const uintptr_t method_descriptor)
	{
		if (!offsets::initialized)
			return;

		if (!hooks::callback::loaded_module)
			hooks::callback::loaded_module = offsets::get_module(method_descriptor);

		hooks::callback::current_method_descriptor = method_descriptor;
		hooks::callback::compiler->compile_method(method_descriptor);
	}

	EXTERNAL bool add_hook(const hooks::callback::p_compilation_callback compilation_callback, 
		const uintptr_t useless_method_desc, const int32_t domain_id, const int32_t runtime_version)
	{
		offsets::runtime_version_major = runtime_version;
		hooks::callback::domain_id = domain_id;
		hooks::callback::compiler = hooks::profiles::create_compiler(runtime_version);
		if (!offsets::initialized && !offsets::initialise_offsets())
			return false;

		hooks::callback::compilation_callback = compilation_callback;
		
		manager = std::make_unique<hooks::hook_manager>(std::vector<std::shared_ptr<hooks::hook>>
		{ 
			{
				static_pointer_cast<hooks::hook>(std::make_shared<hooks::cee_info>()),
				static_pointer_cast<hooks::hook>(hooks::callback::compiler)
			}
		});
		const auto result = manager->apply_hooks();

		hooks::callback::current_method_descriptor = useless_method_desc;
		hooks::callback::compiler->compile_method(useless_method_desc);

		return result;
	}
}