#include <iostream>
#include <thread>
#include <simplistic/fsm.h>

class GreenLightState : public simplistic::fsm::IState {
public:
	void Handle(simplistic::fsm::IContext* ctx);
};
class RedLightState : public simplistic::fsm::IState {
public:
	void Handle(simplistic::fsm::IContext* ctx);
};
class OrangeLightState : public simplistic::fsm::IState {
public:
	void Handle(simplistic::fsm::IContext* ctx);
};

int main()
{
	simplistic::fsm::Context semaphore(
		std::make_unique<RedLightState>());

	while (true)
		semaphore.Handle();
}

void GreenLightState::Handle(simplistic::fsm::IContext* ctx)
{
	std::cout << "Light is Green | Free to go.\n";
	std::this_thread::sleep_for(std::chrono::seconds(5));
	ctx->SetState(std::make_unique<OrangeLightState>());
}

void OrangeLightState::Handle(simplistic::fsm::IContext* ctx)
{
	std::cout << "Light is Orange | Slow Down.\n";
	std::this_thread::sleep_for(std::chrono::seconds(2));
	ctx->SetState(std::make_unique<RedLightState>());
}

void RedLightState::Handle(simplistic::fsm::IContext* ctx)
{
	std::cout << "Light is RED | Stop.\n";
	std::this_thread::sleep_for(std::chrono::seconds(10));
	ctx->SetState(std::make_unique<GreenLightState>());
}