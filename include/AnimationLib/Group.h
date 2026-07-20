///----------------------------------------
/// @file Group.h
/// @ingroup AnimationLib
/// @brief Retained animation group: timed animation storage, update loop, and lifecycle.
/// @details Owns a set of timed animations and advances them each frame via update().
///          The fluent chain-returning entry points (animate()/addAnimator()) are
///          DECLARED here but DEFINED in Chain.h, since they return a @ref chain by
///          value. Include "AnimationLib/Chain.h" (or the umbrella
///          "AnimationLib/Animation.h") to call them; Group.h alone provides the
///          lifecycle API (update, cancel, complete, active, shared, ...).
/// @author Created by John Stephen on 4/19/25.
/// @copyright Copyright © 2025 wobbleworks.com. All rights reserved.
///----------------------------------------

#pragma once

#include "AnimationLib/Curve.h"

#include <functional>
#include <vector>
#include <cassert>
#include <algorithm>
#include <type_traits>
#include <concepts>
#include <utility>
#include <cstdint>
#include <memory>
#include <mutex>
#include <atomic>

///----------------------------------------
namespace Animation {
///----------------------------------------

/// @brief Signature for animation update callbacks receiving elapsed and duration.
using AnimationFunc = std::function<void(double elapsed, double duration)>;

///----------------------------------------
/// @class tag
/// @brief Unique identifier for tagging and cancelling animations.
///----------------------------------------

struct tag {
	uint64_t _id;
	tag() : _id(_nextId.fetch_add(1, std::memory_order_relaxed) + 1) {}
private:
	static inline std::atomic<uint64_t> _nextId{0};
};

// Forward declaration of the fluent builder (defined in Chain.h, befriended below).
class chain;
class group;

///----------------------------------------
/// @struct block_context
/// @brief The defaults an animation block hands to work scheduled inside it.
/// @details Carries the group the block targets, the delay and duration new animations
///          inherit, a tag that groups them for cancel/complete, and an optional easing
///          curve that animated values adopt when they do not name their own.
///----------------------------------------

struct block_context {
	group* _group = nullptr;
	double _delay = 0;
	double _duration = 0;
	uint64_t _tag = 0;
	Curve _curve;
};

///----------------------------------------
/// @class block_scope
/// @brief RAII activation of a @ref block_context on the calling thread.
/// @details Scopes nest; the innermost is active. The destructor restores the previously
///          active scope, so nested blocks — and blocks that exit via an exception — leave
///          the activation stack as they found it. The stack is thread-local, so a block on
///          one thread never disturbs the active context on another.
///----------------------------------------

///----------------------------------------
class block_scope {
///----------------------------------------
	block_context _context;
	block_scope* _previous;
	inline static thread_local block_scope* _current = nullptr;
	
public:
	explicit block_scope(block_context context) noexcept
		: _context(std::move(context)), _previous(_current) {
		_current = this;
	}
	
	~block_scope() {
		_current = _previous;
	}
	
	block_scope(const block_scope&) = delete;
	block_scope& operator=(const block_scope&) = delete;
	
	/// @brief The innermost active context on this thread, or nullptr if none.
	[[nodiscard]] static const block_context* active() noexcept {
		return _current ? &_current->_context : nullptr;
	}
};

///----------------------------------------
/// @class group
/// @brief Retained animation group with chainable sequencing and tagging.
/// @details Manages a set of timed animations. Animations are added via
///          animate() or addAnimator() and updated each frame via update().
///          The chain API enables fluent sequential (.then) and parallel (.with)
///          composition of animations.
/// @note The chain-returning methods are declared here but defined in Chain.h.
///
/// @par Threading and lifetime contract
/// - The animation list is mutex-protected, so scheduling (animate/addAnimator), cancel(),
///   complete(), and update() are each data-race-free from any thread. update() is expected
///   to be driven serially by one render loop (the thread identity may vary — a serial
///   dispatch queue migrates worker threads — but ticks do not overlap).
/// - Callbacks run with the group's mutex released, so a callback may freely schedule,
///   cancel(), or complete() on the same group without deadlocking.
/// - A cross-thread cancel does not interrupt a callback already in flight, so for
///   deterministic ordering cancel from the same context that ticks the group.
/// - An animation added while a tick is in progress first runs on the following tick.
///----------------------------------------

///----------------------------------------
class group : public std::enable_shared_from_this<group> {
///----------------------------------------
	friend class chain;
	
public:
	group() = default;
	
	/// Block-based animate (defined in Chain.h)
	
	///----------------------------------------
	/// @brief Run an animation block using the current default duration.
	/// @tparam F Invocable type (must not be AnimationFunc).
	/// @param block The block to execute within the animation context.
	/// @return A chain for further composition.
	///----------------------------------------
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain animate(F&& block);
	
	///----------------------------------------
	/// @brief Run an animation block with a specified duration.
	/// @tparam F Invocable type (must not be AnimationFunc).
	/// @param duration Duration for animations triggered within the block.
	/// @param block The block to execute within the animation context.
	/// @return A chain for further composition.
	///----------------------------------------
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain animate(double duration, F&& block);
	
	///----------------------------------------
	/// @brief Run an animation block with a specified duration and delay.
	///----------------------------------------
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain animate(double duration, double delay, F&& block);
	
	///----------------------------------------
	/// @brief Run an animation block whose animated values default to the given curve.
	/// @param duration Duration for animations triggered within the block.
	/// @param curve Easing curve adopted by animated values that do not name their own.
	/// @param block The block to execute within the animation context.
	/// @return A chain for further composition.
	///----------------------------------------
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain animate(double duration, Curve curve, F&& block);
	
	///----------------------------------------
	/// @brief Run an animation block with a duration, delay, and default curve.
	///----------------------------------------
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain animate(double duration, double delay, Curve curve, F&& block);
	
	/// AnimationFunc addAnimator (defined in Chain.h)
	
	///----------------------------------------
	/// @brief Add an animator using the current default delay and duration.
	/// @param animation The animation callback.
	/// @return A chain for further composition.
	///----------------------------------------
	
	chain addAnimator(AnimationFunc&& animation);
	
	///----------------------------------------
	/// @brief Add an animator with a specified duration.
	///----------------------------------------
	
	chain addAnimator(double duration, AnimationFunc&& animation);
	
	///----------------------------------------
	/// @brief Add an animator with a specified duration and delay.
	///----------------------------------------
	
	chain addAnimator(double duration, double delay, AnimationFunc&& animation);
	
	/// Tagged overloads (defined in Chain.h)
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain animate(tag t, F&& block);
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain animate(tag t, double duration, F&& block);
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain animate(tag t, double duration, double delay, F&& block);
	
	chain addAnimator(tag t, AnimationFunc&& animation);
	
	chain addAnimator(tag t, double duration, AnimationFunc&& animation);
	
	chain addAnimator(tag t, double duration, double delay, AnimationFunc&& animation);
	
	/// Cancel/complete by tag
	
	///----------------------------------------
	/// @brief Cancel all animations with the given tag without completing them.
	/// @param t Tag identifying the animations to cancel.
	///----------------------------------------
	
	void cancel(tag t) {
		std::lock_guard lock(_mutex);
		// Mark first so a tick already iterating its snapshot skips these before invoking.
		for (auto& animation : _animations) {
			if (animation->_tag == t._id)
				animation->_cancelled = true;
		}
		std::erase_if(_animations, [id = t._id](const AnimationPtr& a) {
			return a->_tag == id;
		});
	}
	
	///----------------------------------------
	/// @brief Complete all animations with the given tag by calling them at full duration.
	/// @param t Tag identifying the animations to complete.
	///----------------------------------------
	
	void complete(tag t) {
		// Extract the matching animations under the lock, then invoke them with the lock
		// released so a completion callback may schedule or cancel on this same group.
		std::vector<AnimationPtr> completing;
		{
			std::lock_guard lock(_mutex);
			for (auto& animation : _animations) {
				if (animation->_tag == t._id) {
					animation->_cancelled = true;
					completing.push_back(animation);
				}
			}
			std::erase_if(_animations, [id = t._id](const AnimationPtr& a) {
				return a->_tag == id;
			});
		}
		for (auto& animation : completing)
			animation->_func(animation->_duration, animation->_duration);
	}
	
	/// Update
	
	///----------------------------------------
	/// @brief Advance all animations to the given time.
	/// @param time Current absolute time.
	///----------------------------------------
	
	void update(double time) {
		std::unique_lock lock(_mutex);
		
		// Note the start time if one hasn't been determined yet
		if (!_started) {
			_startTime = time;
			_started = true;
		}
		_lastUpdateTime = time;
		
		// Calculate the group's local time
		auto groupTime = time - _startTime;
		
		// Iterate a snapshot so a callback may add to or erase from _animations while we
		// run — animations added during this tick first run on the next one.
		auto running = _animations;
		
		for (auto& animation : running) {
			// Skip if a callback earlier this tick cancelled it, or it hasn't started.
			if (animation->_cancelled)
				continue;
			if (groupTime < animation->_delay)
				continue;
				
			// Calculate the animation elapsed
			auto animationTime = groupTime - animation->_delay;
			
			// Invoke with the mutex released so a callback may schedule, cancel, or
			// complete on this same group without deadlocking.
			lock.unlock();
			animation->_func(std::min(animationTime, animation->_duration), animation->_duration);
			lock.lock();
			
			// Remove if completed (a no-op if a callback already removed it).
			if (animationTime >= animation->_duration)
				std::erase(_animations, animation);
		}
		
		// Reset when animations finish
		if (_animations.empty())
			_started = false;
	}
	
	///----------------------------------------
	/// @brief Returns the currently active animation group for this thread.
	///----------------------------------------
	
	[[nodiscard]] static group* active() noexcept {
		auto context = block_scope::active();
		return context ? context->_group : nullptr;
	}
	
	///----------------------------------------
	/// @brief Returns true if no animations are pending.
	///----------------------------------------
	
	[[nodiscard]] bool empty() const {
		std::lock_guard lock(_mutex);
		return _animations.empty();
	}
	
	///----------------------------------------
	/// @brief Returns elapsed time since the group started.
	///----------------------------------------
	
	[[nodiscard]] double elapsed() const {
		std::lock_guard lock(_mutex);
		return _started ? (_lastUpdateTime - _startTime) : 0.0;
	}
	
	///----------------------------------------
	/// @brief Returns the shared global animation group singleton.
	///----------------------------------------
	
	[[nodiscard]] static group* shared() {
		// A process-lifetime singleton, owned by shared_ptr so the group is managed and can hand
		// out shared_from_this() references (e.g. to an animated_value); the raw pointer stays valid
		// for the life of the program.
		static auto sharedGroup = std::make_shared<group>();
		return sharedGroup.get();
	}
	
private:
	mutable std::mutex _mutex;
	
	struct Animation {
		AnimationFunc _func;
		double _delay = 0;
		double _duration = 0;
		uint64_t _tag = 0;
		bool _cancelled = false;
	};
	
	using AnimationPtr = std::shared_ptr<Animation>;
	
	// The delay/duration/tag a defaulting addAnimator inherits: the active block's context
	// when it targets this group, otherwise zeros.
	[[nodiscard]] block_context activeBlockDefaults() noexcept {
		auto context = block_scope::active();
		if (context && context->_group == this)
			return *context;
		return block_context{this, 0.0, 0.0, 0, {}};
	}
	
	// Held by shared_ptr so an entry survives being erased from _animations mid-tick, while a
	// callback runs against a snapshot copy of the vector.
	void addAnimation(Animation animation) {
		std::lock_guard lock(_mutex);
		_animations.push_back(std::make_shared<Animation>(std::move(animation)));
	}
	
	std::vector<AnimationPtr> _animations;
	
	double _startTime = 0;
	double _lastUpdateTime = 0;
	bool _started = false;
};
	
} // namespace
