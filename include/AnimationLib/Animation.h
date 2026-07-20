///----------------------------------------
/// @file Animation.h
/// @ingroup AnimationLib
/// @brief Modern animation group with chainable sequencing and tagging.
/// @details Umbrella header for the modern animation system. Aggregates @ref Group.h
///          (the @ref Animation::group engine) and @ref Chain.h (the @ref
///          Animation::chain fluent builder), and adds the free animate()/addAnimator()
///          convenience functions that target the shared/active group. Existing code
///          should keep including this header; it is the public entry point.
/// @author Created by John Stephen on 4/19/25.
/// @copyright Copyright © 2025 wobbleworks.com. All rights reserved.
///----------------------------------------

#pragma once

#include "AnimationLib/Group.h"
#include "AnimationLib/Chain.h"
#include "AnimationLib/Curve.h"

///----------------------------------------
namespace Animation {
///----------------------------------------

///----------------------------------------
/// MARK: Free functions
///----------------------------------------

///----------------------------------------
/// @brief Run animations within the shared group using the default duration.
///----------------------------------------

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline void animate(F&& block) {
	auto g = group::shared();
	assert(g && "animate() called outside of animation group");
	if (g) {
		g->animate(std::forward<F>(block));
	}
}

///----------------------------------------
/// @brief Run animations within the shared group with a specified duration.
///----------------------------------------

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline void animate(double duration, F&& block) {
	auto g = group::shared();
	assert(g && "animate() called outside of animation group");
	if (g) {
		g->animate(duration, g->elapsed(), std::forward<F>(block));
	}
}

///----------------------------------------
/// @brief Run animations within the shared group with a specified duration and delay.
///----------------------------------------

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline void animate(double duration, double delay, F&& block) {
	auto g = group::shared();
	assert(g && "animate() called outside of animation group");
	if (g) {
		g->animate(duration, delay + g->elapsed(), std::forward<F>(block));
	}
}

///----------------------------------------
/// @brief Run animations within the shared group with a duration and a default curve.
/// @details Animated values set inside the block adopt the given curve unless they name
///          their own.
///----------------------------------------

template <class F> requires (!std::is_same_v<std::decay_t<F>, AnimationFunc>)
                         && std::invocable<F&&>
inline void animate(double duration, Curve curve, F&& block) {
	auto g = group::shared();
	assert(g && "animate() called outside of animation group");
	if (g) {
		g->animate(duration, g->elapsed(), std::move(curve), std::forward<F>(block));
	}
}

///----------------------------------------
/// @brief Add an animator to the currently active group.
///----------------------------------------

inline void addAnimator(AnimationFunc&& animation) {
	auto g = group::active();
	assert(g && "addAnimator() called outside of animation group");
	if (g) {
		g->addAnimator(std::move(animation));
	}
}

///----------------------------------------
/// @brief Add an animator with a specified duration to the currently active group.
///----------------------------------------

inline void addAnimator(double duration, AnimationFunc&& animation) {
	auto g = group::active();
	assert(g && "addAnimator() called outside of animation group");
	if (g) {
		g->addAnimator(duration, std::move(animation));
	}
}

///----------------------------------------
/// @brief Add an animator with a specified duration and delay to the currently active group.
///----------------------------------------

inline void addAnimator(double duration, double delay, AnimationFunc&& animation) {
	auto g = group::active();
	assert(g && "addAnimator() called outside of animation group");
	if (g) {
		g->addAnimator(duration, delay, std::move(animation));
	}
}
	
} // namespace
