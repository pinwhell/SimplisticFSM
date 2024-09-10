#pragma once

#include <memory>
#ifdef SFSM_MT
#include <mutex>
#endif

namespace simplistic { namespace fsm {
	class IContext;
	class IState {
	public:
		virtual ~IState() = default;
		virtual void Handle(IContext* ctx) = 0;
	};

	class IContext {
	public:
		virtual ~IContext() = default;
		virtual void SetState(std::unique_ptr<IState> newState, bool bImmidiateOverride = false) = 0;
		virtual void Handle() = 0;
	};

	class Context : public IContext {
	public:
		inline Context() = default;
		inline Context(std::unique_ptr<IState> initialState)
			: mCurrent(std::move(initialState))
		{}

		inline void SetState(std::unique_ptr<IState> newState, bool bImmidiateOverride = false)
		{
#ifdef SFSM_MT
			std::lock_guard<std::mutex> queueLock(mQueueMutex);
#endif
			auto& dst = bImmidiateOverride ? mCurrent : mQueuedNext;
			dst = std::move(newState);
		}

		inline void Handle()
		{
			FlushQueue();

			if (mCurrent)
				mCurrent->Handle(this);
		}

		inline void FlushQueue()
		{
#ifdef SFSM_MT
			std::lock_guard<std::mutex> queueLock(mQueueMutex);
#endif

			if (!mQueuedNext)
				return;

			// realizing the queued to current
			mCurrent = std::move(mQueuedNext);
			mQueuedNext.reset();
		}

		std::unique_ptr<IState> mCurrent;
		std::unique_ptr<IState> mQueuedNext;
#ifdef SFSM_MT
		std::mutex mQueueMutex;
#endif
	};
}
}