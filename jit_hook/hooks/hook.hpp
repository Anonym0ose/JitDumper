#pragma once
namespace jit_hook::hooks
{
	class hook
	{
	public:
		virtual void apply_hook() const = 0;

		virtual void remove_hook() const = 0;
	};
}