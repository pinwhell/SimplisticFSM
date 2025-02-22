#pragma once

#include <memory>
#include <functional>
#include <variant>
#include <optional>
#include <type_traits>
#include <mutex>

namespace simplistic {
	namespace fsm {
		class IContext;

		class IState {
		public:
			inline virtual ~IState() = default;
			inline virtual void Enter(IContext* ctx) {};
			inline virtual void operator()(IContext* ctx) {}
			inline virtual void Exit(IContext* ctx) {};
		};

		struct SegmentedHandle {
			using Handle = std::function<void(IContext*)>;

			template <typename T>
			inline SegmentedHandle(T&& handle, std::optional<Handle> enter = {}, std::optional<Handle> exit = {})
				: mEnter(std::move(enter)), mHandle(std::forward<T>(handle)), mExit(std::move(exit)) {}

			inline SegmentedHandle(SegmentedHandle&& other) noexcept
				: mEnter(std::move(other.mEnter))
				, mHandle(std::move(other.mHandle))
				, mExit(std::move(other.mExit)) {}

			inline SegmentedHandle& operator=(SegmentedHandle&& other) noexcept {
				if (this != &other) {
					mEnter = std::move(other.mEnter);
					mHandle = std::move(other.mHandle);
					mExit = std::move(other.mExit);
				}
				return *this;
			}

			inline void Enter(IContext* ctx) { if (mEnter) (*mEnter)(ctx); }
			inline void operator()(IContext* ctx) { mHandle(ctx); }
			inline void Exit(IContext* ctx) { if (mExit) (*mExit)(ctx); }

			std::optional<Handle> mEnter;
			Handle mHandle;
			std::optional<Handle> mExit;
		};

		struct UnifiedHandle {
			enum class HandleReason { ENTER, HANDLE, EXIT };
			using Handle = std::function<void(IContext*, HandleReason)>;

			template <typename T>
			inline UnifiedHandle(T&& handler) : mHandle(std::forward<T>(handler)) {}

			inline UnifiedHandle(UnifiedHandle&& other) noexcept
				: mHandle(std::move(other.mHandle)) {}

			inline UnifiedHandle& operator=(UnifiedHandle&& other) noexcept {
				if (this != &other) mHandle = std::move(other.mHandle);
				return *this;
			}

			inline void Enter(IContext* ctx) { mHandle(ctx, HandleReason::ENTER); }
			inline void operator()(IContext* ctx) { mHandle(ctx, HandleReason::HANDLE); }
			inline void Exit(IContext* ctx) { mHandle(ctx, HandleReason::EXIT); }

			Handle mHandle;
		};

		struct InterfacedHandle {
			using Storage = std::variant<std::unique_ptr<IState>, std::shared_ptr<IState>, IState*>;

			template <typename T>
			inline InterfacedHandle(T&& state)
				: mStateStorage(std::move(state)), mState(GetBasePtr()) {}

			inline InterfacedHandle(InterfacedHandle&& other) noexcept
				: mStateStorage(std::move(other.mStateStorage))
				, mState(GetBasePtr()) {}

			inline InterfacedHandle& operator=(InterfacedHandle&& other) noexcept {
				if (this != &other) {
					mStateStorage = std::move(other.mStateStorage);
					mState = GetBasePtr();
				}
				return *this;
			}

			inline void Enter(IContext* ctx) { mState->Enter(ctx); }
			inline void operator()(IContext* ctx) { (*mState)(ctx); }
			inline void Exit(IContext* ctx) { mState->Exit(ctx); }

		private:
			inline IState* GetBasePtr() {
				return std::visit([](auto& s) -> IState* {
					if constexpr (std::is_pointer_v<std::decay_t<decltype(s)>>) return s;
					else return s.get();
					}, mStateStorage);
			}

			Storage mStateStorage;
			IState* mState;
		};

		struct Handle {
			using Any = std::variant<
				UnifiedHandle,
				InterfacedHandle,
				SegmentedHandle>;
			template <typename T>
			inline explicit Handle(T&& handle)
				: mHandle(std::move(handle)) {}

			inline Handle(Handle&& other) noexcept : mHandle(std::move(other.mHandle)) {}

			inline Handle& operator=(Handle&& other) noexcept
			{
				if (this != &other) mHandle = std::move(other.mHandle);
				return *this;
			}

			inline void Enter(IContext* ctx)
			{
				std::visit([ctx](auto& handle) {
					handle.Enter(ctx);
					}, mHandle);
			}

			inline void operator()(IContext* ctx)
			{
				std::visit([this, ctx](auto& handle) {
					handle(ctx);
					}, mHandle);
			}

			inline void Exit(IContext* ctx)
			{
				std::visit([ctx](auto& handle) {
					handle.Exit(ctx);
					}, mHandle);
			}

			Handle::Any mHandle;
		};

		class IContext {
		public:
			inline IContext() {}
			inline virtual ~IContext() = default;
			inline virtual void Apply(std::optional<Handle>&& newState, bool immApply = false) = 0;
			inline void Apply(std::unique_ptr<IState> state, bool immApply = false) {
				Apply(Handle(InterfacedHandle(std::move(state))), immApply);  // Calls the Handle constructor accepting unique_ptr
			}
			inline void ApplyShared(std::shared_ptr<IState> state, bool immApply = false) {
				Apply(Handle(InterfacedHandle(state)), immApply);  // Calls the Handle constructor accepting shared_ptr
			}
			inline void ApplyNull(bool immApply = false) { Apply(std::optional<Handle>{}, immApply); }
			inline virtual void operator()() {}
		};

		class Context : public IContext {
		public:
			using AnyState = std::variant<fsm::Handle, fsm::Handle*>;

			inline Context() : mCurrent(nullptr), mNextQueue(nullptr) {}
			inline Context(fsm::Handle&& initialState)
				: mCurrent(std::move(initialState))
				, mNextQueue(nullptr)
			{}
			using IContext::Apply;
			inline virtual void Apply(std::optional<Handle>&& newState, bool immApply = false)
			{
#ifdef SFSM_MT
				std::lock_guard<std::mutex> queueLock(mQueueMutex);
#endif
				auto& dst = immApply ? mCurrent : mNextQueue;
				if (immApply) if (auto* currState = GetState(dst)) currState->Exit(this);
				dst = newState ? std::move(*newState) : AnyState(nullptr);
				if (immApply) if (auto* currState = GetState(dst)) currState->Enter(this);
			}

			inline void operator()()
			{
				DoApply();
				if (auto* state = GetState(mCurrent)) (*state)(this);
			}

			inline void DoApply()
			{
#ifdef SFSM_MT
				std::lock_guard<std::mutex> queueLock(mQueueMutex);
#endif
				if (!GetState(mNextQueue)) return;
				if (auto* currState = GetState(mCurrent)) currState->Exit(this);
				if (auto* currState = (GetState(mCurrent =
					std::move(mNextQueue)))) currState->Enter(this);
				mNextQueue = nullptr;
			}

			static fsm::Handle* GetState(AnyState& state)
			{
				return std::visit([](auto& curr) -> fsm::Handle* {
					using TQueue = std::remove_reference_t<decltype(curr)>;
					if constexpr (std::is_pointer_v<TQueue>)
						return curr;
					else return &curr;
					}, state);
			}

			AnyState mCurrent;
			AnyState mNextQueue;
#ifdef SFSM_MT
			std::mutex mQueueMutex;
#endif
		};
	}
}

#define SFSM_UNIFIED(state) simplistic::fsm::Handle( \
simplistic::fsm::UnifiedHandle(state))

#define SFSM_UNIFIEDWTHIS2(_this, state) SFSM_UNIFIED( \
std::bind( \
	state, \
	(_this), \
	std::placeholders::_1, \
	std::placeholders::_2) \
)

#define SFSM_UNIFIEDWTHIS(state) SFSM_UNIFIEDWTHIS2(this, (state))