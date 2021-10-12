#pragma once

namespace jit_hook::hooks
{
	class compiler;
}

namespace jit_hook::hooks::callback
{
	struct eh_clause
	{
		int32_t flags;
		uint32_t try_offset;
		uint32_t try_length;
		uint32_t handler_offset;
		uint32_t handler_length;
		uint32_t class_token_or_filter_offset;
	};

	struct jit_info
	{
		uintptr_t locals_sig;
		uintptr_t cil_code;
		uintptr_t eh_clauses;
		size_t cil_code_size;
		size_t exception_handlers_count;
		size_t locals_size;
	};

	using p_compilation_callback = void(__cdecl*)(jit_info&&);
	p_compilation_callback compilation_callback;

	int32_t domain_id;
	uintptr_t current_method_descriptor;
	uintptr_t loaded_module;

	std::shared_ptr<jit_hook::hooks::compiler> compiler;

	std::unordered_map<uintptr_t, std::vector<uint8_t>> locals_from_method_desc;
	std::unordered_map<uintptr_t, uint32_t> tokens_from_handle;
	std::unordered_map<uint32_t, uintptr_t> handles_from_token;

}