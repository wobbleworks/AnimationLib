///----------------------------------------
/// @file Chain.h
/// @ingroup AnimationLib
/// @brief Fluent builder for sequencing and parallelizing animations within a group.
/// @details Defines the @ref chain class returned by group::animate()/addAnimator(),
///          providing sequential (.then) and parallel (.with) composition. This header
///          ALSO completes @ref group: its chain-returning methods are defined
///          out-of-line below, since they require @ref chain to be a complete type.
/// @author Created by John Stephen on 4/19/25.
/// @copyright Copyright © 2025 wobbleworks.com. All rights reserved.
///----------------------------------------

#pragma once

#include "AnimationLib/Group.h"
#include "AnimationLib/SelfTestCheck.h"

///----------------------------------------
namespace Animation {
///----------------------------------------

///----------------------------------------
/// @class chain
/// @brief Fluent builder for sequencing and parallelizing animations within a group.
/// @details A chain is a statement-scoped builder: it holds a raw pointer to the group it
///          schedules into and must not outlive the full expression that produced it. Build
///          and consume it in one statement (group->animate(...).then(...).with(...)); do not
///          store one for later use.
///----------------------------------------

///----------------------------------------
class chain {
///----------------------------------------
	group* _group;
	double _cursor;     // max end time — where .then() continues from
	double _lastStart;  // where the last animation started — where .with() starts
	double _duration;   // carried forward for convenience
	uint64_t _tag;      // tag for cancel/complete (0 = untagged)
	
public:
	chain(group* g, double cursor, double lastStart, double duration, uint64_t tag = 0)
		: _group(g), _cursor(cursor), _lastStart(lastStart), _duration(duration), _tag(tag) {}
		
	/// AnimationFunc overloads — sequential
	
	///----------------------------------------
	/// @brief Append an animation sequentially after the current cursor.
	/// @param duration Duration of the animation.
	/// @param func Animation callback.
	/// @return Updated chain for further composition.
	///----------------------------------------
	
	chain then(double duration, AnimationFunc&& func) {
		_group->addAnimation(group::Animation{std::move(func), _cursor, duration, _tag});
		return {_group, _cursor + duration, _cursor, duration, _tag};
	}
	
	///----------------------------------------
	/// @brief Append an animation sequentially using the current duration.
	///----------------------------------------
	
	chain then(AnimationFunc&& func) {
		return then(_duration, std::move(func));
	}
	
	///----------------------------------------
	/// @brief Append an animation after a delay from the current cursor.
	///----------------------------------------
	
	chain thenAfterDelay(double delay, double duration, AnimationFunc&& func) {
		auto start = _cursor + delay;
		_group->addAnimation(group::Animation{std::move(func), start, duration, _tag});
		return {_group, start + duration, start, duration, _tag};
	}
	
	///----------------------------------------
	/// @brief Append an animation after a delay using the current duration.
	///----------------------------------------
	
	chain thenAfterDelay(double delay, AnimationFunc&& func) {
		return thenAfterDelay(delay, _duration, std::move(func));
	}
	
	/// AnimationFunc overloads — parallel
	
	///----------------------------------------
	/// @brief Add an animation in parallel with the last animation.
	///----------------------------------------
	
	chain with(double duration, AnimationFunc&& func) {
		_group->addAnimation(group::Animation{std::move(func), _lastStart, duration, _tag});
		return {_group, std::max(_cursor, _lastStart + duration), _lastStart, duration, _tag};
	}
	
	///----------------------------------------
	/// @brief Add an animation in parallel using the current duration.
	///----------------------------------------
	
	chain with(AnimationFunc&& func) {
		return with(_duration, std::move(func));
	}
	
	///----------------------------------------
	/// @brief Add an animation in parallel after a delay.
	///----------------------------------------
	
	chain withAfterDelay(double delay, double duration, AnimationFunc&& func) {
		auto start = _lastStart + delay;
		_group->addAnimation(group::Animation{std::move(func), start, duration, _tag});
		return {_group, std::max(_cursor, start + duration), _lastStart, duration, _tag};
	}
	
	///----------------------------------------
	/// @brief Add an animation in parallel after a delay using the current duration.
	///----------------------------------------
	
	chain withAfterDelay(double delay, AnimationFunc&& func) {
		return withAfterDelay(delay, _duration, std::move(func));
	}
	
	/// Block overloads — sequential
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain then(double duration, F&& block) {
		block_scope scope({_group, _cursor, duration, _tag, {}});
		std::forward<F>(block)();
		return {_group, _cursor + duration, _cursor, duration, _tag};
	}
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain then(F&& block) {
		return then(_duration, std::forward<F>(block));
	}
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain thenAfterDelay(double delay, double duration, F&& block) {
		auto start = _cursor + delay;
		block_scope scope({_group, start, duration, _tag, {}});
		std::forward<F>(block)();
		return {_group, start + duration, start, duration, _tag};
	}
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain thenAfterDelay(double delay, F&& block) {
		return thenAfterDelay(delay, _duration, std::forward<F>(block));
	}
	
	/// Block overloads — parallel
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain with(double duration, F&& block) {
		block_scope scope({_group, _lastStart, duration, _tag, {}});
		std::forward<F>(block)();
		return {_group, std::max(_cursor, _lastStart + duration), _lastStart, duration, _tag};
	}
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain with(F&& block) {
		return with(_duration, std::forward<F>(block));
	}
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain withAfterDelay(double delay, double duration, F&& block) {
		auto start = _lastStart + delay;
		block_scope scope({_group, start, duration, _tag, {}});
		std::forward<F>(block)();
		return {_group, std::max(_cursor, start + duration), _lastStart, duration, _tag};
	}
	
	template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
	                         && std::invocable<F&&>
	chain withAfterDelay(double delay, F&& block) {
		return withAfterDelay(delay, _duration, std::forward<F>(block));
	}
};

///----------------------------------------
/// MARK: group chain-returning methods
///----------------------------------------

/// Block-based animate

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline chain group::animate(F&& block) {
	// No duration given: inherit the enclosing block's duration for this group, else zero.
	auto defaults = activeBlockDefaults();
	return animate(defaults._duration, std::forward<F>(block));
}

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline chain group::animate(double duration, F&& block) {
	block_scope scope({this, 0.0, duration, 0, {}});
	std::forward<F>(block)();
	return {this, duration, 0.0, duration, 0};
}

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline chain group::animate(double duration, double delay, F&& block) {
	block_scope scope({this, delay, duration, 0, {}});
	std::forward<F>(block)();
	return {this, delay + duration, delay, duration, 0};
}

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline chain group::animate(double duration, Curve curve, F&& block) {
	block_scope scope({this, 0.0, duration, 0, std::move(curve)});
	std::forward<F>(block)();
	return {this, duration, 0.0, duration, 0};
}

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline chain group::animate(double duration, double delay, Curve curve, F&& block) {
	block_scope scope({this, delay, duration, 0, std::move(curve)});
	std::forward<F>(block)();
	return {this, delay + duration, delay, duration, 0};
}

/// AnimationFunc addAnimator

inline chain group::addAnimator(AnimationFunc&& animation) {
	auto defaults = activeBlockDefaults();
	addAnimation(Animation{std::move(animation), defaults._delay, defaults._duration, defaults._tag});
	return {this, defaults._delay + defaults._duration, defaults._delay, defaults._duration, defaults._tag};
}

inline chain group::addAnimator(double duration, AnimationFunc&& animation) {
	auto defaults = activeBlockDefaults();
	addAnimation(Animation{std::move(animation), defaults._delay, duration, defaults._tag});
	return {this, defaults._delay + duration, defaults._delay, duration, defaults._tag};
}

inline chain group::addAnimator(double duration, double delay, AnimationFunc&& animation) {
	auto defaults = activeBlockDefaults();
	addAnimation(Animation{std::move(animation), delay, duration, defaults._tag});
	return {this, delay + duration, delay, duration, defaults._tag};
}

/// Tagged overloads

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline chain group::animate(tag t, F&& block) {
	auto defaults = activeBlockDefaults();
	return animate(t, defaults._duration, std::forward<F>(block));
}

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline chain group::animate(tag t, double duration, F&& block) {
	block_scope scope({this, 0.0, duration, t._id, {}});
	std::forward<F>(block)();
	return {this, duration, 0.0, duration, t._id};
}

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline chain group::animate(tag t, double duration, double delay, F&& block) {
	block_scope scope({this, delay, duration, t._id, {}});
	std::forward<F>(block)();
	return {this, delay + duration, delay, duration, t._id};
}

inline chain group::addAnimator(tag t, AnimationFunc&& animation) {
	auto defaults = activeBlockDefaults();
	addAnimation(Animation{std::move(animation), defaults._delay, defaults._duration, t._id});
	return {this, defaults._delay + defaults._duration, defaults._delay, defaults._duration, t._id};
}

inline chain group::addAnimator(tag t, double duration, AnimationFunc&& animation) {
	auto defaults = activeBlockDefaults();
	addAnimation(Animation{std::move(animation), defaults._delay, duration, t._id});
	return {this, defaults._delay + duration, defaults._delay, duration, t._id};
}

inline chain group::addAnimator(tag t, double duration, double delay, AnimationFunc&& animation) {
	addAnimation(Animation{std::move(animation), delay, duration, t._id});
	return {this, delay + duration, delay, duration, t._id};
}

///----------------------------------------
/// MARK: Self-tests
///----------------------------------------

///----------------------------------------
/// @brief Exercises group scheduling, the mutex-released callback contract, and tags.
/// @details Drives a group with synthetic times (no wall clock): checks delay/elapsed/clamp
///          scheduling and removal on completion; confirms a callback may cancel another
///          animation on the same group without deadlocking and that the cancelled victim
///          never runs; confirms an animation added from within a tick first runs on the
///          next tick; and confirms complete() invokes once at full duration.
///----------------------------------------

inline void groupSelfTest() {
	using Animation::selftest::check;
	
	// Scheduling: an animation runs from its delay, clamps elapsed to duration, and is removed.
	{
		auto g = std::make_shared<group>();
		auto calls = 0;
		auto lastElapsed = -1.0;
		auto lastDuration = -1.0;
		g->addAnimator(1.0, 0.5, [&](double elapsed, double duration) {
			++calls;
			lastElapsed = elapsed;
			lastDuration = duration;
		});
		g->update(100.0);
		check(calls == 0, "an animation does not run before its delay");
		g->update(100.5);
		check(calls == 1 && lastElapsed == 0.0 && lastDuration == 1.0, "an animation starts at its delay");
		g->update(101.0);
		check(std::abs(lastElapsed - 0.5) < 1.0e-9, "elapsed advances with time");
		g->update(101.5);
		check(std::abs(lastElapsed - 1.0) < 1.0e-9, "elapsed clamps to duration on the final tick");
		check(g->empty(), "a completed animation is removed");
	}
	
	// A callback may cancel another animation on the same group; the victim never runs.
	{
		auto g = std::make_shared<group>();
		tag victimTag;
		auto cancellerRan = false;
		auto victimRan = false;
		// Canceller first so it precedes the victim in the tick's snapshot.
		g->addAnimator(1.0, 0.0, [&](double, double) {
			cancellerRan = true;
			g->cancel(victimTag);
		});
		g->addAnimator(victimTag, 1.0, 0.0, [&](double, double) {
			victimRan = true;
		});
		g->update(0.0);
		check(cancellerRan, "a callback that cancels its neighbour still runs");
		check(!victimRan, "cancelling from a callback skips the not-yet-run victim");
	}
	
	// An animation added from within a tick first runs on the following tick.
	{
		auto g = std::make_shared<group>();
		auto addedRan = false;
		auto adderCalls = 0;
		g->addAnimator(2.0, 0.0, [&](double, double) {
			if (++adderCalls == 1)
				g->addAnimator(2.0, 0.0, [&](double, double) { addedRan = true; });
		});
		g->update(0.0);
		check(!addedRan, "an animation added during a tick does not run in that tick");
		g->update(0.5);
		check(addedRan, "an animation added during a tick runs on the next tick");
	}
	
	// complete() invokes matching animations once at full duration and removes them.
	{
		auto g = std::make_shared<group>();
		tag t;
		auto completeCalls = 0;
		auto completeElapsed = -1.0;
		auto completeDuration = -1.0;
		g->addAnimator(t, 2.0, 0.0, [&](double elapsed, double duration) {
			++completeCalls;
			completeElapsed = elapsed;
			completeDuration = duration;
		});
		g->complete(t);
		check(completeCalls == 1 && completeElapsed == 2.0 && completeDuration == 2.0, "complete() invokes once at full duration");
		check(g->empty(), "complete() removes the animation");
	}
}

///----------------------------------------
/// @brief Exercises the chain builder's sequencing against an analytic and explicit schedule.
/// @details Builds addAnimator(1).then(2).with(1).thenAfterDelay(0.5) and recovers each
///          animation's start time from its first callback (start = time − elapsed). The
///          starts must match both the hand-computed schedule and an equivalent schedule
///          assembled with explicit addAnimator(duration, delay) calls. Also confirms the
///          active block context is restored after a block, including one that throws.
///----------------------------------------

inline void chainSelfTest() {
	using Animation::selftest::check;
	
	// Recovers each animation's start time (time − elapsed) into starts[index].
	auto recorder = [](double* starts, int index, const double* clock) {
		return [starts, index, clock](double elapsed, double) {
			if (starts[index] < 0.0)
				starts[index] = *clock - elapsed;
		};
	};
	
	double chainStarts[4] = {-1, -1, -1, -1};
	double chainTime = 0;
	auto chained = std::make_shared<group>();
	chained->addAnimator(1.0, recorder(chainStarts, 0, &chainTime))
		.then(2.0, recorder(chainStarts, 1, &chainTime))
		.with(1.0, recorder(chainStarts, 2, &chainTime))
		.thenAfterDelay(0.5, recorder(chainStarts, 3, &chainTime));
	for (chainTime = 0.0; chainTime <= 5.0; chainTime += 0.125)
		chained->update(chainTime);
		
	double explicitStarts[4] = {-1, -1, -1, -1};
	double explicitTime = 0;
	auto spelled = std::make_shared<group>();
	spelled->addAnimator(1.0, 0.0, recorder(explicitStarts, 0, &explicitTime));
	spelled->addAnimator(2.0, 1.0, recorder(explicitStarts, 1, &explicitTime));
	spelled->addAnimator(1.0, 1.0, recorder(explicitStarts, 2, &explicitTime));
	spelled->addAnimator(1.0, 3.5, recorder(explicitStarts, 3, &explicitTime));
	for (explicitTime = 0.0; explicitTime <= 5.0; explicitTime += 0.125)
		spelled->update(explicitTime);
		
	const double expected[4] = {0.0, 1.0, 1.0, 3.5};
	for (auto i = 0; i < 4; ++i) {
		check(std::abs(chainStarts[i] - expected[i]) < 1.0e-9, "the chain schedule matches the analytic start time");
		check(std::abs(chainStarts[i] - explicitStarts[i]) < 1.0e-9, "the chain schedule matches explicit addAnimator");
	}
	
	// The active block context is restored after a block, including one that throws.
	check(block_scope::active() == nullptr, "no block context is active at rest");
	auto scoped = std::make_shared<group>();
	scoped->animate(1.0, [&] {
		auto context = block_scope::active();
		check(context != nullptr && context->_group == scoped.get(), "the block's context is active inside it");
	});
	check(block_scope::active() == nullptr, "the block context is cleared after the block");
	try {
		scoped->animate(1.0, [] { throw std::runtime_error("boom"); });
	} catch (...) {
	}
	check(block_scope::active() == nullptr, "the block context is cleared after a throwing block");
	
	// Blocks are independent scopes: a nested un-tagged block does not inherit the outer
	// block's tag, so an outer cancel(tag) leaves animators scheduled inside it untouched.
	{
		auto g = std::make_shared<group>();
		tag outer;
		auto innerRan = false;
		g->animate(outer, 2.0, [&] {
			g->animate(1.0, [&] {
				g->addAnimator(1.0, [&](double, double) { innerRan = true; });
			});
		});
		g->cancel(outer);
		g->update(0.0);
		g->update(1.0);
		check(innerRan, "a nested un-tagged block does not inherit the outer tag");
	}
}
	
} // namespace
