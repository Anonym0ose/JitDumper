#pragma once

namespace jit_hook::pdb
{
	class pdb_process
	{
	public:
		pdb_process(HANDLE handle) : process_handle{ handle }
		{
			SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_PUBLICS_ONLY | SYMOPT_UNDNAME);
			const auto symbols_path = std::filesystem::current_path() / "Symbols";
			SymInitialize(process_handle, symbols_path.string().c_str(), false);
		}

		~pdb_process()
		{
			SymCleanup(process_handle);
		}

		std::shared_ptr<pdb_module> create_module(const std::string_view module_name)
		{
			const auto module = std::make_shared<pdb_module>(module_name, process_handle);
			modules.push_back(module);
			return module;
		}

		template<typename T>
		T get_function_address(const std::string_view function_name) const
		{
			auto symbol_info = SYMBOL_INFO{ sizeof(SYMBOL_INFO) };

			if (!SymFromName(process_handle, function_name.data(), &symbol_info))
			{
				throw std::invalid_argument(std::format("Failed to get the address of the function {}", function_name));
			}

			return T(symbol_info.Address);
		}
		
	private:
		std::vector<std::shared_ptr<pdb_module>> modules;
		HANDLE process_handle;
	};
}