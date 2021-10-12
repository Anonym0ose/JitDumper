#pragma once
namespace jit_hook::hooks
{
	class cee_info : public hook
	{
	public:
		void apply_hook() const override
		{
			auto resolve_token_address = &cee_info::resolve_token_hook;
			DetourAttach(&reinterpret_cast<void*&>(offsets::resolve_token), *reinterpret_cast<void**>(&resolve_token_address));

			auto get_arg_type_address = &cee_info::get_arg_type_hook;
			DetourAttach(&reinterpret_cast<void*&>(offsets::get_arg_type), *reinterpret_cast<void**>(&get_arg_type_address));
		}

		void remove_hook() const override
		{
			auto resolve_token_address = &cee_info::resolve_token_hook;
			DetourDetach(&reinterpret_cast<void*&>(offsets::resolve_token), *reinterpret_cast<void**>(&resolve_token_address));

			auto get_arg_type_address = &cee_info::get_arg_type_hook;
			DetourDetach(&reinterpret_cast<void*&>(offsets::get_arg_type), *reinterpret_cast<void**>(&get_arg_type_address));
		}

	private:
		void resolve_token_hook(const uintptr_t resolved_token)
		{
			offsets::resolve_token(this, resolved_token);
			const auto token = read<uint32_t>(resolved_token + offsets::resolved_token_offsets.at("token"));
			const auto module = read<uint32_t>(resolved_token + offsets::resolved_token_offsets.at("tokenScope"));

			if (callback::loaded_module == module)
			{
				callback::tokens_from_handle[get_handle(resolved_token)] = token;
			}
		}

		int32_t get_arg_type_hook(const uintptr_t sig_info, uint8_t* local_sig, uintptr_t* type_handle)
		{
			const auto result = offsets::get_arg_type(this, sig_info, local_sig, type_handle);
			const auto method_info = sig_info - offsets::method_info_offsets.at("locals");
			const auto method_desc = read<uintptr_t>(method_info + offsets::method_info_offsets.at("ftn"));

			if (method_desc && method_desc == callback::current_method_descriptor)
			{
				const auto sig_size = offsets::get_arg_next(this, local_sig) - local_sig;
				for (auto i = 0; i < sig_size; ++i)
				{
					callback::locals_from_method_desc[method_desc].push_back(*(local_sig + i));
				}
			}

			return result;
		}
	};
}