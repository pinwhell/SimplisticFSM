#pragma once

#include <memory>
#ifdef SFSM_MT
#include <mutex>
#endif
#include <functional>
#include <variant>
#include <optional>
#include <type_traits>

namespace simplistic { namespace fsm {
	class IContext;

	struct State {

		using SimpleHandle = std::function<void(IContext*)>;
		using FullHandle = std::function<void(IContext*, bool)>;

		inline State(IContext* ctx,
			std::function<void(IContext*)> handle,
			std::function<void(IContext*)> enter,
			std::function<void(IContext*)> exit)
			: mContext(ctx)
			, mEnter(enter)
			, mHandle(handle)
			, mExit(exit)
		{
			if(mEnter) mEnter(ctx);
		}

		inline State(IContext* ctx,
			std::function<void(IContext*, bool)> handle)
			: mContext(ctx)
			, mEnter(0)
			, mHandle(handle)
			, mExit(0)
		{
			handle(mContext, true);
		}

		void Handle(IContext* ctx)
		{
			std::visit([this](auto cb) {
				using StateHandle = std::decay_t<decltype(cb)>;
				if (!cb) return;
				if constexpr (std::is_same_v<StateHandle, SimpleHandle>)
					cb(mContext);
				else 
					cb(mContext, false);
				}, mHandle);
		}

		~State()
		{
			if(mExit) mExit(mContext);
		}

		IContext* mContext;
		std::function<void(IContext*)> mEnter;
		std::variant<SimpleHandle, FullHandle> mHandle;
		std::function<void(IContext*)> mExit;
	};

	class IState {
	public:
		virtual ~IState() = default;
		virtual void Handle(IContext* ctx) = 0;
	};

	class IContext {
	public:
		virtual ~IContext() = default;
		virtual void SetState(std::variant<IState*, State>&& newState, bool bImmidiateOverride = false) = 0;
		virtual void SetState(std::unique_ptr<IState> newState, bool bImmidiateOverride = false) = 0;
		virtual void Handle() = 0;
	};

	class Context : public IContext {
	public:
		inline Context() = default;
		inline Context(std::unique_ptr<IState> initialState)
			: mCurrent(initialState.get())
			, mCurrentStg(std::move(initialState))
		{}
		inline Context(std::variant<IState*, State>&& initialState)
			: mCurrent(std::move(initialState))
		{}

		inline void SetState(std::variant<IState*, State>&& newState, bool bImmidiateOverride = false)
		{
#ifdef SFSM_MT
			std::lock_guard<std::mutex> queueLock(mQueueMutex);
#endif
			auto& dst = bImmidiateOverride ? mCurrent : mQueuedNext;
			dst = std::move(newState);
		}

		inline void SetState(std::unique_ptr<IState> newState, bool bImmidiateOverride = false)
		{
#ifdef SFSM_MT
			std::lock_guard<std::mutex> queueLock(mQueueMutex);
#endif

			auto& dst = bImmidiateOverride ? mCurrent : mQueuedNext;
			auto& dstStg = bImmidiateOverride ? mCurrentStg : mQueuedStg;
			dst = newState.get();
			dstStg = std::move(newState);
		}

		inline void Handle()
		{
			FlushQueue();

			if (mCurrent) std::visit([this](auto& state) {
				using StateType = std::decay_t<decltype(state)>;
				if constexpr (std::is_same_v<StateType, State>)
					state.Handle(this);
				else 
					state->Handle(this);
				}, *mCurrent);
		}

		inline void FlushQueue()
		{
#ifdef SFSM_MT
			std::lock_guard<std::mutex> queueLock(mQueueMutex);
#endif

			if (!mQueuedNext)
				return;

			// realizing the queued to current
			*mCurrent = std::move(*mQueuedNext);
			if (mQueuedStg) mCurrentStg = std::move(mQueuedStg);
			mQueuedNext.reset();
			mQueuedStg.reset();
		}

		std::optional<std::variant<IState*, State>> mCurrent;
		std::optional<std::variant<IState*, State>> mQueuedNext;
		std::unique_ptr<IState> mCurrentStg;
		std::unique_ptr<IState> mQueuedStg;
#ifdef SFSM_MT
		std::mutex mQueueMutex;
#endif
	};
}
}

#define SFSM_CTXSTATE(ctx, state, _this) State(ctx, std::bind( \
	&state,\
	_this,\
	std::placeholders::_1, \
	std::placeholders::_2))

#define SFSM_THISCTXSTATE(state) SFSM_CTXSTATE(this, state, this)