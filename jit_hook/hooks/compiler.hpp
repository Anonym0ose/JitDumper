#pragma once

namespace jit_hook::hooks
{
	class compiler : public hook
	{
	public:
		virtual void compile_method(const uintptr_t method_desc) const = 0;

		void apply_hook() const override
		{
			auto comp_compile_address = &compiler::comp_compile;
			DetourAttach(&reinterpret_cast<void*&>(offsets::comp_compile), *reinterpret_cast<void**>(&comp_compile_address));

			auto imp_resolve_token_address = &compiler::imp_resolve_token;
			DetourAttach(&reinterpret_cast<void*&>(offsets::imp_resolve_token),
				*reinterpret_cast<void**>(&imp_resolve_token_address));
		}

		void remove_hook() const override
		{
			auto comp_compile_address = &compiler::comp_compile;
			DetourDetach(&reinterpret_cast<void*&>(offsets::comp_compile), *reinterpret_cast<void**>(&comp_compile_address));

			auto imp_resolve_token_address = &compiler::imp_resolve_token;
			DetourDetach(&reinterpret_cast<void*&>(offsets::imp_resolve_token),
				*reinterpret_cast<void**>(&imp_resolve_token_address));
		}

	private:
		inline static uintptr_t fake_code;

		virtual void import_phase(uintptr_t compiler) const = 0;

		void comp_compile(uintptr_t* method_code, uint32_t* code_size, const uintptr_t compile_flags)
		{
			const auto compiler = reinterpret_cast<uintptr_t>(this);
			const auto info = compiler + offsets::compiler_offsets.at("info");
			const auto method_descriptor = read<uintptr_t>(info + offsets::info_offsets.at("compMethodHnd"));

			if (callback::current_method_descriptor == method_descriptor && fake_code == 0)
			{
				offsets::comp_compile(this, method_code, code_size, compile_flags);
				fake_code = *method_code;
				return;
			}

			if (callback::current_method_descriptor == method_descriptor)
			{
				callback::compiler->import_phase(compiler);
				call_callback(info);
				*method_code = fake_code; // CLR Exception is thrown if the pointer is 0
				return;
			}

			offsets::comp_compile(this, method_code, code_size, compile_flags);
		}

		void imp_resolve_token(uint32_t* token, const uintptr_t resolved_token, const int32_t kind)
		{
			offsets::imp_resolve_token(this, token, resolved_token, kind);
			const auto module = read<uintptr_t>(resolved_token + offsets::resolved_token_offsets.at("tokenScope"));

			if (callback::loaded_module == module)
			{
				callback::handles_from_token[*token] = get_handle(resolved_token);
			}
		}

		void call_callback(const uintptr_t info) const
		{
			const auto il_code = read<uintptr_t>(info + offsets::info_offsets.at("compCode"));
			const auto il_code_size = read<uint32_t>(info + offsets::info_offsets.at("compILCodeSize"));
			const auto& locals = callback::locals_from_method_desc[callback::current_method_descriptor];
			const auto clauses = get_eh_info();

			callback::compilation_callback({ reinterpret_cast<uintptr_t>(locals.data()), il_code,
				reinterpret_cast<uintptr_t>(clauses.data()), il_code_size, clauses.size(), locals.size() });
		}

		std::vector<callback::eh_clause> get_eh_info() const
		{
			const auto compiler = reinterpret_cast<uintptr_t>(this);
			auto eh_table = read<uintptr_t>(compiler + offsets::compiler_offsets.at("compHndBBtab"));
			const auto eh_table_count = read<uint32_t>(compiler + offsets::compiler_offsets.at("compHndBBtabCount"));

			const auto flags_name = offsets::new_eh_block_struct ? "ebdHandlerType" : "ebdFlags";
			auto clauses = std::vector<callback::eh_clause>{};

			for (auto i = 0u; i < eh_table_count; ++i, eh_table += offsets::eh_block_class_size)
			{
				auto flags = read<int32_t>(eh_table + offsets::eh_block_offsets.at(flags_name));
				if (offsets::new_eh_block_struct)
					flags = to_eh_clause_flags(flags);

				const auto try_start = read<uint32_t>(eh_table + offsets::eh_block_offsets.at("ebdTryBegOffset"));
				const auto try_length = read<uint32_t>(eh_table + offsets::eh_block_offsets.at("ebdTryEndOffset")) - try_start;
				const auto filter_offset = read<uint32_t>(eh_table + offsets::eh_block_offsets.at("ebdFilterBegOffset"));
				const auto class_token = read<uint32_t>(eh_table + offsets::eh_block_offsets.at("ebdTyp"));
				const auto handler_start = read<uint32_t>(eh_table + offsets::eh_block_offsets.at("ebdHndBegOffset"));
				const auto handler_length = 
					read<uint32_t>(eh_table + offsets::eh_block_offsets.at("ebdHndEndOffset")) - handler_start;

				clauses.emplace_back(flags, try_start, try_length, handler_start, handler_length,
					flags & 0x1 ? filter_offset : class_token);
			}

			return clauses;
		}
	};
}