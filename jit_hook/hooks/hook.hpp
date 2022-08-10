#pragma once
namespace jit_hook::hooks
{
	class hook
	{
	public:
		virtual ~hook() {};

		virtual bool apply_hook() const = 0;
	};
}