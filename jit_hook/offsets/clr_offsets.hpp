#pragma once
#include <unordered_map>
#include <iostream>
#include "pdb/pdb_module.hpp"
#include "pdb/pdb_process.hpp"
#include "pdb/pdb_downloader.hpp"

namespace jit_hook::offsets
{
#pragma region Jit Compilation
	using p_comp_compile = void(__thiscall*)(void* compiler, uintptr_t* method_code, uint32_t* code_size, uintptr_t flags);
	using p_get_module = uintptr_t(__thiscall*)(uintptr_t method_desc);
	using p_jit_and_collect_trace_net_4 = void(__thiscall*)(void* stack_sampler, uintptr_t method_desc, int32_t* adid);
	using p_jit_and_collect_trace_net_core_3 = void(__thiscall*)(void* stack_sampler, uintptr_t method_desc);
#pragma endregion
#pragma region Reading Local Signatures
	using p_get_arg_next = uint8_t*(__thiscall*)(void* cee_info, uint8_t* locals_sig);
	using p_get_arg_type = int(__thiscall*)(void* cee_info, uintptr_t sig_info, uint8_t* locals_sig, uintptr_t* type_handle);
#pragma endregion
#pragma region Resolving Tokens
	using p_imp_import_net_4 = void(__thiscall*)(uintptr_t compiler, uintptr_t block);
	using p_imp_import_net_core_5 = void(__thiscall*)(uintptr_t compiler);
	using p_resolve_token = void(__thiscall*)(void* cee_info, uintptr_t resolved_token);
	using p_imp_resolve_token = void(__thiscall*)(void* compiler, uint32_t* token, uintptr_t resolved_token, int32_t kind);
#pragma endregion
#pragma region Misc
	using p_is_instantiating_stub = int32_t(__thiscall*)(uintptr_t method_desc);
	using p_get_wrapped_method_descriptor = uintptr_t(__thiscall*)(uintptr_t method_desc);
#pragma endregion

	p_comp_compile comp_compile;
	p_get_module get_module;
	uintptr_t jit_and_collect_trace;

	p_get_arg_next get_arg_next;
	p_get_arg_type get_arg_type;

	uintptr_t imp_import;
	p_resolve_token resolve_token;
	p_imp_resolve_token imp_resolve_token;

	p_is_instantiating_stub is_instantiating_stub;
	p_get_wrapped_method_descriptor get_wrapped_method_descriptor;

	// https://github.com/dotnet/runtime/blob/b6647fce59c7de0f870dae4bd008c278c99e036f/src/coreclr/jit/compiler.h#L9997
	// https://github.com/dotnet/runtime/blob/b6647fce59c7de0f870dae4bd008c278c99e036f/src/coreclr/jit/compiler.h#L9687
	// https://github.com/dotnet/runtime/blob/57bfe474518ab5b7cfe6bf7424a79ce3af9d6657/src/coreclr/jit/jiteh.h#L80
	// https://github.com/dotnet/runtime/blob/f96ca33fbcd41407cc564a48917b3a84c4c21d26/src/coreclr/inc/corinfo.h#L1121
	// https://github.com/dotnet/runtime/blob/f96ca33fbcd41407cc564a48917b3a84c4c21d26/src/coreclr/inc/corinfo.h#L1097
	// https://github.com/dotnet/runtime/blob/f96ca33fbcd41407cc564a48917b3a84c4c21d26/src/coreclr/inc/corinfo.h#L1527
	std::unordered_map<std::string, std::ptrdiff_t> compiler_offsets;
	std::unordered_map<std::string, std::ptrdiff_t> info_offsets;
	std::unordered_map<std::string, std::ptrdiff_t> eh_block_offsets;
	std::unordered_map<std::string, std::ptrdiff_t> method_info_offsets;
	std::unordered_map<std::string, std::ptrdiff_t> sig_info_offsets;
	std::unordered_map<std::string, std::ptrdiff_t> resolved_token_offsets;
	std::unordered_map<std::string, std::ptrdiff_t> basic_block_offsets;

	bool new_eh_block_struct;
	size_t eh_block_class_size;
	size_t runtime_version_major;

	bool initialized;

	p_comp_compile get_comp_compile_address(const std::shared_ptr<pdb::pdb_module>& pdb)
	{
		auto symbols = std::vector<std::pair<std::string, uint64_t>>{};
		pdb->enumerate_symbols("*compCompile*", &symbols);

		for (const auto& [symbol, address] : symbols)
		{
			if (symbol.find("void __") != std::string::npos &&
				symbol.find("Compiler::compCompile") != std::string::npos)
				return reinterpret_cast<p_comp_compile>(address);
		}

		throw std::invalid_argument("Failed to get the address of the function Compiler::compCompile");
	}

	bool initialise_offsets()
	{
		if (initialized)
			return true;

		try
		{
			std::cout << "Loading PDBs...\n";

			pdb::download_pdb(runtime_version_major == 4 ? "clr.dll" : "coreclr.dll");
			pdb::download_pdb("clrjit.dll");

			auto process = pdb::pdb_process{ GetCurrentProcess() };
			process.create_module(runtime_version_major == 4 ? "clr.dll" : "coreclr.dll");
			const auto clr_jit = process.create_module("clrjit.dll");
			std::cout << "Loaded PDBs\n";

			comp_compile = get_comp_compile_address(clr_jit);
			get_module = process.get_function_address<p_get_module>("MethodDesc::GetModule");
			jit_and_collect_trace = process.get_function_address<uintptr_t>("StackSampler::JitAndCollectTrace");

			get_arg_next = process.get_function_address<p_get_arg_next>("CEEInfo::getArgNext");
			get_arg_type = process.get_function_address<p_get_arg_type>("CEEInfo::getArgType");

			imp_import = process.get_function_address<uintptr_t>("Compiler::impImport");
			resolve_token = process.get_function_address<p_resolve_token>("CEEInfo::resolveToken");
			imp_resolve_token = process.get_function_address<p_imp_resolve_token>("Compiler::impResolveToken");

			is_instantiating_stub = process.get_function_address<p_is_instantiating_stub>("MethodDesc::IsInstantiatingStub");
			get_wrapped_method_descriptor =
				process.get_function_address<p_get_wrapped_method_descriptor>("MethodDesc::GetWrappedMethodDesc");

			new_eh_block_struct = clr_jit->type_exists("EHblkDsc");
			const auto eh_block_dsc_name = new_eh_block_struct ? "EHblkDsc" : "Compiler::EHblkDsc";
			eh_block_class_size = clr_jit->get_type_size(eh_block_dsc_name);

			compiler_offsets =
			{
				{"info", -1},
				{"compHndBBtab", -1},
				{"compHndBBtabCount", -1},
				{"fgFirstBB", -1}
			};
			clr_jit->get_offsets("Compiler", compiler_offsets);

			info_offsets =
			{
				{"compCode", -1},
				{"compILCodeSize", -1},
				{"compMethodHnd", -1}
			};
			clr_jit->get_offsets("Compiler::Info", info_offsets);

			eh_block_offsets =
			{
				{"ebdTyp", -1},
				{new_eh_block_struct ? "ebdHandlerType" : "ebdFlags", -1},
				{"ebdTryBegOffset", -1},
				{"ebdTryEndOffset", -1},
				{"ebdFilterBegOffset", -1},
				{"ebdHndBegOffset", -1},
				{"ebdHndEndOffset", -1}
			};
			clr_jit->get_offsets(eh_block_dsc_name, eh_block_offsets);

			method_info_offsets =
			{
				{"locals", -1},
				{"ftn", -1}
			};
			clr_jit->get_offsets("CORINFO_METHOD_INFO", method_info_offsets);

			sig_info_offsets =
			{
				{"args", -1},
				{"numArgs", -1}
			};
			clr_jit->get_offsets("CORINFO_SIG_INFO", sig_info_offsets);

			resolved_token_offsets =
			{
				{"hClass", -1},
				{"hMethod", -1},
				{"hField", -1},
				{"tokenScope", -1},
				{"token", -1},
			};
			clr_jit->get_offsets("CORINFO_RESOLVED_TOKEN", resolved_token_offsets);

			basic_block_offsets =
			{
				{"bbNext", -1},
				{"bbFlags", -1}
			};
			clr_jit->get_offsets("BasicBlock", basic_block_offsets);

			return initialized = true;
		}
		catch (const std::invalid_argument& exception)
		{
			std::cout << exception.what() << "\n";
			return false;
		}
	}
}
