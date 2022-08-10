#pragma once
namespace jit_hook::hooks
{
	class cee_info : public hook
	{
	public:
		~cee_info() override
		{
			resolve_token_manager.reset();
			get_arg_type_manager.reset();
		}

		bool apply_hook() const override
		{
			auto resolve_token_address = &cee_info::resolve_token_hook;
			resolve_token_manager = std::make_unique<PLH::HWBreakPointHook>(reinterpret_cast<char*>(offsets::resolve_token),
				*reinterpret_cast<char**>(&resolve_token_address), GetCurrentThread());

			auto get_arg_type_address = &cee_info::get_arg_type_hook;
			get_arg_type_manager = std::make_unique<PLH::HWBreakPointHook>(reinterpret_cast<char*>(offsets::get_arg_type),
				*reinterpret_cast<char**>(&get_arg_type_address), GetCurrentThread());

			return resolve_token_manager->hook() && get_arg_type_manager->hook();
		}

	private:
		inline static std::unique_ptr<PLH::HWBreakPointHook> resolve_token_manager;
		inline static std::unique_ptr<PLH::HWBreakPointHook> get_arg_type_manager;

		void resolve_token_hook(const uintptr_t resolved_token)
		{
			auto protection_object = resolve_token_manager->getProtectionObject();

			offsets::resolve_token(this, resolved_token);
			const auto token = read<uint32_t>(resolved_token + offsets::resolved_token_offsets.at("token"));
			const auto module = read<uintptr_t>(resolved_token + offsets::resolved_token_offsets.at("tokenScope"));

			if (callback::loaded_module == module)
			{
				callback::tokens_from_handle[get_handle(resolved_token)] = token;
			}
		}

		int32_t get_arg_type_hook(const uintptr_t sig_info, uint8_t* local_sig, uintptr_t* type_handle)
		{
			auto protection_object = get_arg_type_manager->getProtectionObject();

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