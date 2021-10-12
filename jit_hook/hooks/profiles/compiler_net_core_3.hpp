#pragma once

namespace jit_hook::hooks::profiles
{
	class compiler_net_core_3 : public compiler
	{
	public:
		void compile_method(const uintptr_t method_desc) const override
		{
			auto stack_sampler = std::array<uint8_t, 200>();
			reinterpret_cast<offsets::p_jit_and_collect_trace_net_core_3>(
				offsets::jit_and_collect_trace)(stack_sampler.data(), method_desc);
		}

	private:
		void import_phase(const uintptr_t compiler) const override
		{
			const auto basic_block = read<uintptr_t>(compiler + offsets::compiler_offsets.at("fgFirstBB"));
			reinterpret_cast<offsets::p_imp_import_net_4>(offsets::imp_import)(compiler, basic_block);
		}
	};
}