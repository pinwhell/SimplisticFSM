#include <iostream>
#include <thread>
//#include <simplistic/fsm.h>
#include "../include/simplistic/fsm.h"

using namespace simplistic::fsm;

class SemaphoreController : public simplistic::fsm::Context {
public:
	SemaphoreController()
		: simplistic::fsm::Context(SFSM_THISCTXSTATE(SemaphoreController::GreenLightState))
	{}

	void GreenLightState(simplistic::fsm::IContext* ctx, bool entry)
	{
		if (entry) return;
		std::cout << "Light is Green | Free to go.\n";
		std::this_thread::sleep_for(std::chrono::seconds(5));
		ctx->SetState(std::make_unique<OrangeLightState>(this));
	}

	class OrangeLightState : public simplistic::fsm::IState {
	public:
		OrangeLightState(SemaphoreController* crller)
			: mController(crller)
		{}

		void Handle(simplistic::fsm::IContext* ctx)
		{
			std::cout << "Light is Orange | Slow Down.\n";
			std::this_thread::sleep_for(std::chrono::seconds(2));
			ctx->SetState(SFSM_CTXSTATE(
				ctx,
				SemaphoreController::RedLightState,
				mController));
		}
		SemaphoreController* mController;
	};

	void RedLightState(simplistic::fsm::IContext* ctx, bool entry)
	{
		if (entry) return;
		std::cout << "Light is RED | Stop.\n";
		std::this_thread::sleep_for(std::chrono::seconds(10));
		ctx->SetState(SFSM_THISCTXSTATE(SemaphoreController::GreenLightState));
	}
};

int main()
{
	SemaphoreController semCtrller;

	while (true)
		semCtrller.Handle();
}