#pragma once
namespace jit_hook::hooks
{
	template<typename T>
	T read(uintptr_t address)
	{
		return *reinterpret_cast<T*>(address);
	}

	template<typename T>
	void write(uintptr_t address, T value)
	{
		*reinterpret_cast<T*>(address) = value;
	}

	inline uintptr_t get_handle(const uintptr_t resolved_token)
	{
		const auto type_handle = read<uintptr_t>(resolved_token + offsets::resolved_token_offsets.at("hClass"));
		const auto method_handle = read<uintptr_t>(resolved_token + offsets::resolved_token_offsets.at("hMethod"));
		const auto field_handle = read<uintptr_t>(resolved_token + offsets::resolved_token_offsets.at("hField"));

		if (method_handle)
			return method_handle;

		if (field_handle)
			return field_handle;

		return type_handle;
	}

	// https://github.com/dotnet/runtime/blob/b6647fce59c7de0f870dae4bd008c278c99e036f/src/coreclr/jit/jiteh.h#L35
	inline int32_t to_eh_clause_flags(const int32_t eh_handler_type)
	{
		switch (eh_handler_type)
		{
		case 0x1:
			return 0; // CORINFO_EH_CLAUSE_NONE
		case 0x2:
			return 0x1; // CORINFO_EH_CLAUSE_FILTER
		case 0x3:
		case 0x5:
			return 0x4; // CORINFO_EH_CLAUSE_FAULT
		case 0x4:
			return 0x2; // CORINFO_EH_CLAUSE_FINALLY
		default:
			return -1;
		}
	}
}