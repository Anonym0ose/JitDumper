#pragma once
#include "hooks/profiles/compiler.hpp"
#include "hooks/profiles/compiler_net_4.hpp"
#include "hooks/profiles/compiler_net_core_1.hpp"
#include "hooks/profiles/compiler_net_core_2.hpp"
#include "hooks/profiles/compiler_net_core_3.hpp"
#include "hooks/profiles/compiler_net_core_5.hpp"

namespace jit_hook::hooks::profiles
{
	inline std::shared_ptr<compiler> create_compiler(const int32_t runtime_version)
	{
		switch (runtime_version)
		{
		case 1:
			return std::make_shared<compiler_net_core_1>();
		case 2:
			return std::make_shared<compiler_net_core_2>();
		case 3:
			return std::make_shared<compiler_net_core_3>();
		case 4:
			return std::make_shared<compiler_net_4>();
		case 5:
			return std::make_shared<compiler_net_core_5>();
		default:
			return nullptr;
		}
	}
}
