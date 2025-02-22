#include <iostream>
#include <thread>
#include <simplistic/fsm.h>

using namespace simplistic::fsm;
using SFMUnifiedReason = simplistic::fsm::UnifiedHandle::HandleReason;

class SemaphoreController : public simplistic::fsm::Context {
public:
	SemaphoreController()
		: simplistic::fsm::Context(SFSM_UNIFIEDWTHIS(&SemaphoreController::GreenLightState))
	{}

	void GreenLightState(simplistic::fsm::IContext* ctx, SFMUnifiedReason reason)
	{
		if (reason == SFMUnifiedReason::ENTER) return;
		std::cout << "Light is Green | Free to go.\n";
		std::this_thread::sleep_for(std::chrono::seconds(5));
		ctx->Apply(std::make_unique<OrangeLightState>(this));
	}

	class OrangeLightState : public simplistic::fsm::IState {
	public:
		OrangeLightState(SemaphoreController* crller)
			: mController(crller)
		{}

		void operator()(simplistic::fsm::IContext* ctx)
		{
			std::cout << "Light is Orange | Slow Down.\n";
			std::this_thread::sleep_for(std::chrono::seconds(2));
			ctx->Apply(SFSM_UNIFIEDWTHIS2(mController,
				&SemaphoreController::RedLightState));
		}
		SemaphoreController* mController;
	};

	void RedLightState(simplistic::fsm::IContext* ctx, SFMUnifiedReason reason)
	{
		if (reason == SFMUnifiedReason::ENTER) return;
		std::cout << "Light is RED | Stop.\n";
		std::this_thread::sleep_for(std::chrono::seconds(10));
		ctx->Apply(SFSM_UNIFIEDWTHIS(&SemaphoreController::GreenLightState));
	}
};

int main()
{
	SemaphoreController semCtrller;

	while (true)
		semCtrller();
}