#pragma once
namespace jit_hook::hooks
{
	class hook_manager
	{
	public:
		hook_manager(std::vector<std::shared_ptr<hook>>&& instances) : hooks(std::move(instances))
		{

		}

		~hook_manager()
		{
			hooks.clear();
		}

		bool apply_hooks() const
		{
			auto result = true;
			for (const auto& hook : hooks)
			{
				result &= hook->apply_hook();
			}

			return result;
		}

	private:
		std::vector<std::shared_ptr<hook>> hooks;
	};
}