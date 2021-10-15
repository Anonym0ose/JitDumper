#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#include <filesystem>
#include <format>
#include <array>
#include <locale>
#include <codecvt>

namespace jit_hook::pdb
{
	class pdb_module
	{
	public:
		pdb_module(const std::string_view module_name, const HANDLE process) : owner_process{ process }
		{
			const auto module_handle = reinterpret_cast<DWORD64>(GetModuleHandleA(module_name.data()));
			loaded_module = SymLoadModuleEx(owner_process, INVALID_HANDLE_VALUE, module_name.data(), nullptr,
				module_handle, 0, nullptr, 0);

			if (!loaded_module && GetLastError() == ERROR_SUCCESS)
				loaded_module = module_handle;
		}

		static BOOL CALLBACK enum_sym_proc(SYMBOL_INFO* symbol_info, ULONG, void* user_context)
		{
			auto undecorated_name = std::array<char, MAX_SYM_NAME>{};
			UnDecorateSymbolName(symbol_info->Name, undecorated_name.data(), MAX_SYM_NAME, UNDNAME_COMPLETE);

			const auto symbols = reinterpret_cast<std::vector<std::pair<std::string, uint64_t>>*>(user_context);
			symbols->emplace_back(undecorated_name.data(), symbol_info->Address);
			return TRUE;
		}

		bool enumerate_symbols(const std::string_view mask, std::vector<std::pair<std::string, uint64_t>>* user_context) const
		{
			const auto old_options = SymGetOptions();
			SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_PUBLICS_ONLY);
			const auto result = SymEnumSymbols(owner_process, loaded_module, mask.data(), enum_sym_proc, user_context);

			SymSetOptions(old_options);
			return result;
		}

		bool type_exists(const std::string_view type_name) const
		{
			auto type_symbol = SYMBOL_INFO{ sizeof(SYMBOL_INFO) };
			return SymGetTypeFromName(owner_process, loaded_module, type_name.data(), &type_symbol);
		}

		size_t get_type_size(const std::string_view type_name) const
		{
			auto type_symbol = SYMBOL_INFO{ sizeof(SYMBOL_INFO) };

			if (!SymGetTypeFromName(owner_process, loaded_module, type_name.data(), &type_symbol))
			{
				throw std::invalid_argument(std::format("Failed to get the size of type {}", type_name));
			}

			auto size = uint64_t{ 0 };
			SymGetTypeInfo(owner_process, loaded_module, type_symbol.TypeIndex, TI_GET_LENGTH, &size);
			return static_cast<size_t>(size);
		}

		void get_offsets(const std::string_view type_name, std::unordered_map<std::string, std::ptrdiff_t>& offsets) const
		{
			auto type_symbol = SYMBOL_INFO{ sizeof(SYMBOL_INFO) };

			if (!SymGetTypeFromName(owner_process, loaded_module, type_name.data(), &type_symbol))
			{
				throw std::invalid_argument(std::format("Failed to get the type index of {}", type_name));
			}

			auto members_count = 0u;
			SymGetTypeInfo(owner_process, loaded_module, type_symbol.TypeIndex, TI_GET_CHILDRENCOUNT, &members_count);

			const auto buffer = std::make_unique<char[]>((members_count + 2) * sizeof(uint32_t));
			const auto members = reinterpret_cast<TI_FINDCHILDREN_PARAMS*>(buffer.get());
			members->Count = members_count;
			SymGetTypeInfo(owner_process, loaded_module, type_symbol.TypeIndex, TI_FINDCHILDREN, members);

			for (auto i = members->Start; i < members->Count; ++i)
			{
				auto member_name_buffer = nullptr;
				if (!SymGetTypeInfo(owner_process, loaded_module, members->ChildId[i], TI_GET_SYMNAME, &member_name_buffer))
					continue;

				const auto member_name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(member_name_buffer);
				LocalFree(member_name_buffer);
				if (offsets.contains(member_name))
				{
					auto offset = 0u;
					SymGetTypeInfo(owner_process, loaded_module, members->ChildId[i], TI_GET_OFFSET, &offset);
					offsets[member_name] = offset;
				}
			}

			for (const auto& [member, offset] : offsets)
			{
				if (offset == -1)
				{
					throw std::invalid_argument(std::format("Failed to get the offset of the member {} of type {}",
						member, type_name));
				}
			}
		}
	
	private:
		DWORD64 loaded_module;
		const HANDLE owner_process;
	};
}