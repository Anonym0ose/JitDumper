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
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			for (const auto& hook : hooks)
			{
				hook->remove_hook();
			}

			DetourTransactionCommit();
		}

		bool apply_hooks() const
		{
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			for (const auto& hook : hooks)
			{
				hook->apply_hook();
			}

			return DetourTransactionCommit() == 0;
		}

	private:
		std::vector<std::shared_ptr<hook>> hooks;
	};
}