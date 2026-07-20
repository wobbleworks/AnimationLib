///----------------------------------------
/// @file Value.h
/// @ingroup AnimationLib
/// @brief Animated value that transitions smoothly within animation groups.
/// @details Wraps a value of type T with automatic animation support. When
///          setValue() is called inside an animation block, the value transitions
///          from its current state to the target over the block's duration.
/// @author Created by John Stephen on 2/27/26.
/// @copyright Copyright © 2026 wobbleworks.com. All rights reserved.
///----------------------------------------

#pragma once

#include "AnimationLib/Animation.h"
#include "AnimationLib/SelfTestCheck.h"

#include <atomic>

///----------------------------------------
namespace Animation {
///----------------------------------------

///----------------------------------------
/// @brief Compare two values for equality
/// @tparam T Value type.
/// @param a First value.
/// @param b Second value.
/// @return True if the values are equal.
///----------------------------------------

template <class T>
[[nodiscard]] inline bool valuesEqual(const T& a, const T& b) { return a == b; }

///----------------------------------------
/// @class animated_value
/// @brief A value that animates smoothly when modified inside an animation block.
/// @tparam T Value type supporting arithmetic operations.
/// @details The in-flight value and animating flag live in a heap @ref State block held by a
///          shared_ptr. The animator scheduled by setValue() captures a copy of that
///          shared_ptr, so a callback in flight during a tick keeps the State alive and writes
///          it safely even if the owning animated_value is destroyed on another thread
///          meanwhile — the destructor cancels future callbacks but never frees the block out
///          from under a running one. The value is written during update() and read without
///          synchronization, which suits the render-loop read pattern used throughout, but a
///          wide T could tear if written and read from different threads at once.
///----------------------------------------

///----------------------------------------
template <class T>
class animated_value {
///----------------------------------------
	// Mutable animation state, shared with the in-flight animator so its lifetime is
	// independent of the owning animated_value.
	struct State {
		T value{};
		std::atomic<bool> animating{false};
	};
	
	std::shared_ptr<State> _state = std::make_shared<State>();
	T _targetValue{};
	std::shared_ptr<group> _group;
	tag _tag;
	
public:
	animated_value() = default;
	
	animated_value(T initial) : _targetValue(initial) {
		_state->value = initial;
	}
	
	animated_value(const animated_value& other) : _targetValue(other._targetValue) {
		_state->value = other._targetValue;
	}
	
	~animated_value() {
		cancelAnimations();
	}
	
	animated_value& operator=(const animated_value& other) {
		if (this != &other) {
			cancelAnimations();
			_targetValue = other._targetValue;
			_state->value = other._targetValue;
		}
		return *this;
	}
	
	///----------------------------------------
	/// @brief Cancel any in-progress animations.
	/// @param finish If true, jump to the end value before cancelling.
	///----------------------------------------
	
	void cancelAnimations(bool finish = false) {
		if (_state->animating && _group) {
			if (finish)
				_group->complete(_tag);
			else
				_group->cancel(_tag);
			_state->animating = false;
		}
	}
	
	///----------------------------------------
	/// @brief Set the value, animating if inside an animation block.
	/// @param newValue The target value.
	/// @param animated Whether to animate (ignored if not in an animation block).
	/// @param curve The easing curve to use; an empty curve adopts the enclosing block's
	///        curve when it supplied one, otherwise curves::linear.
	/// @return True if the value is animating.
	///----------------------------------------
	
	bool setValue(T newValue, bool animated = true, Curve curve = {}) {
		// Leave if it's the same target value
		if (valuesEqual(_targetValue, newValue))
			return false;
			
		// Save the new target
		_targetValue = newValue;
		
		// Cancel any existing animations
		cancelAnimations();
		
		// Start a new animation if inside an animation block
		auto g = group::active();
		if (g && animated) {
			// Resolve the curve once, at schedule time: an explicit curve wins, then the
			// enclosing block's curve, then a plain linear.
			if (!curve) {
				auto context = block_scope::active();
				curve = (context && context->_curve) ? context->_curve : curves::linear;
			}
			
			// Hold a strong reference so the group outlives an in-flight animation on this value.
			_group = g->shared_from_this();
			_tag = tag();
			_state->animating = true;
			
			// Capture a copy of the shared state (not this): the animator writes through it, so
			// it stays valid even if the owning value is destroyed on another thread mid-tick.
			auto state = _state;
			auto fromValue = _state->value;
			auto toValue = newValue;
			g->addAnimator(_tag, [state, fromValue, toValue, curve](double elapsed, double duration) {
				auto factor = (duration > 0) ? (elapsed / duration) : 1.0;
				// Qualified so argument-dependent lookup does not also pull in Math::lerp for
				// vector value types; a type may still customize via an Animation::lerp overload.
				state->value = Animation::lerp(fromValue, toValue, curve(factor));
				if (elapsed >= duration)
					state->animating = false;
			});
			
			return true;
			
		// Just use the new value if not animating
		} else {
			_state->value = newValue;
			return false;
		}
	}
	
	///----------------------------------------
	/// @brief Set the value immediately without animation, cancelling any in progress.
	/// @param newValue The value to set.
	///----------------------------------------
	
	void setImmediate(T newValue) {
		cancelAnimations();
		_state->value = newValue;
		_targetValue = newValue;
	}
	
	///----------------------------------------
	/// @brief Update the target value, and the current value if not animating.
	/// @param newValue The new target value.
	///----------------------------------------
	
	void setTargetValue(T newValue) {
		_targetValue = newValue;
		if (!_state->animating)
			_state->value = newValue;
	}
	
	/// @brief Whether the value is currently animating.
	[[nodiscard]] bool isAnimating() const noexcept { return _state->animating; }
	
	/// @brief The current in-flight animated value.
	[[nodiscard]] T animatedValue() const noexcept { return _state->value; }
	
	/// @brief The target value the animation is heading towards.
	[[nodiscard]] T targetValue() const noexcept { return _targetValue; }
	
	/// @brief Implicit conversion returns the target value.
	operator T() const noexcept { return _targetValue; }
	
	animated_value& operator=(T newValue) {
		setValue(newValue);
		return *this;
	}
};

using animated_float = animated_value<float>;
using animated_double = animated_value<double>;

///----------------------------------------
/// MARK: Self-test
///----------------------------------------

///----------------------------------------
/// @brief Exercises animated_value's snap, interpolation, retarget, cancel, and curve paths.
/// @details Drives values on a local group with synthetic times: confirms setValue snaps
///          outside a block; a linear animation is halfway at half the duration and lands on
///          its target; a mid-flight retarget continues from the current value; cancel with
///          and without finish; a block default curve produces the same trajectory as an
///          explicit curve (a differential check of the curve-resolution path); and that
///          destroying an animating value cancels its animation.
///----------------------------------------

inline void animatedValueSelfTest() {
	using Animation::selftest::check;
	
	// Outside any block, setValue snaps immediately.
	{
		animated_float value(0.0f);
		value.setValue(1.0f);
		check(!value.isAnimating() && value.animatedValue() == 1.0f, "setValue snaps outside an animation block");
	}
	
	// Inside a block, a linear animation interpolates and lands on its target.
	{
		auto g = std::make_shared<group>();
		animated_float value(0.0f);
		g->animate(1.0, g->elapsed(), curves::linear, [&] { value.setValue(10.0f); });
		check(value.isAnimating(), "setValue inside a block animates");
		g->update(0.0);
		g->update(0.5);
		check(std::abs(value.animatedValue() - 5.0f) < 1.0e-4f, "a linear animation is halfway at half the duration");
		g->update(1.0);
		check(std::abs(value.animatedValue() - 10.0f) < 1.0e-4f && !value.isAnimating(), "a linear animation lands on its target");
		check(float(value) == 10.0f, "operator T reads the target value");
	}
	
	// A mid-flight retarget continues from the current value.
	{
		auto g = std::make_shared<group>();
		animated_float value(0.0f);
		g->animate(1.0, g->elapsed(), curves::linear, [&] { value.setValue(10.0f); });
		g->update(0.0);
		g->update(0.5);
		auto mid = value.animatedValue();
		check(mid > 4.0f && mid < 6.0f, "the value is mid-way before retargeting");
		g->animate(1.0, g->elapsed(), curves::linear, [&] { value.setValue(0.0f); });
		g->update(0.5);
		check(std::abs(value.animatedValue() - mid) < 1.0e-4f, "a retarget starts from the current value");
		g->update(1.5);
		check(std::abs(value.animatedValue() - 0.0f) < 1.0e-4f && !value.isAnimating(), "a retarget lands on the new target");
	}
	
	// cancelAnimations(finish=true) lands on the target; (false) freezes the current value.
	{
		auto g = std::make_shared<group>();
		animated_float value(0.0f);
		g->animate(1.0, g->elapsed(), curves::linear, [&] { value.setValue(10.0f); });
		g->update(0.0);
		g->update(0.5);
		value.cancelAnimations(true);
		check(std::abs(value.animatedValue() - 10.0f) < 1.0e-4f && !value.isAnimating(), "cancelAnimations(true) lands on the target");
	}
	{
		auto g = std::make_shared<group>();
		animated_float value(0.0f);
		g->animate(1.0, g->elapsed(), curves::linear, [&] { value.setValue(10.0f); });
		g->update(0.0);
		g->update(0.5);
		auto frozen = value.animatedValue();
		value.cancelAnimations(false);
		check(std::abs(value.animatedValue() - frozen) < 1.0e-4f && !value.isAnimating(), "cancelAnimations(false) freezes the current value");
	}
	
	// A block default curve drives a plain setValue exactly as an explicit curve would.
	{
		auto blockGroup = std::make_shared<group>();
		auto explicitGroup = std::make_shared<group>();
		animated_float viaBlock(0.0f);
		animated_float viaExplicit(0.0f);
		blockGroup->animate(1.0, curves::easeOut, [&] { viaBlock.setValue(10.0f); });
		explicitGroup->animate(1.0, [&] { viaExplicit.setValue(10.0f, true, curves::easeOut); });
		for (auto step = 0; step <= 10; ++step) {
			auto time = step / 10.0;
			blockGroup->update(time);
			explicitGroup->update(time);
			check(std::abs(viaBlock.animatedValue() - viaExplicit.animatedValue()) < 1.0e-4f, "a block default curve matches an explicit curve");
		}
	}
	
	// Destroying an animating value cancels its animation.
	{
		auto g = std::make_shared<group>();
		{
			animated_float value(0.0f);
			g->animate(1.0, g->elapsed(), curves::linear, [&] { value.setValue(10.0f); });
			g->update(0.0);
			check(!g->empty(), "the group holds the animation while the value lives");
		}
		check(g->empty(), "destroying an animating value cancels its animation");
	}
	
	// A zero-duration animation lands on its target on the first tick and is removed.
	{
		auto g = std::make_shared<group>();
		animated_float value(0.0f);
		g->animate(0.0, g->elapsed(), curves::linear, [&] { value.setValue(10.0f); });
		g->update(0.0);
		check(std::abs(value.animatedValue() - 10.0f) < 1.0e-4f && !value.isAnimating(), "a zero-duration animation lands immediately");
		check(g->empty(), "a zero-duration animation is removed");
	}
	
	// An overshooting curve carries the value past its target mid-flight, then lands exactly.
	{
		auto g = std::make_shared<group>();
		animated_float value(0.0f);
		g->animate(1.0, g->elapsed(), curves::springIn, [&] { value.setValue(10.0f); });
		g->update(0.0);
		g->update(0.75);
		check(value.animatedValue() > 10.0f, "an overshooting curve carries the value past its target");
		g->update(1.0);
		check(std::abs(value.animatedValue() - 10.0f) < 1.0e-4f, "an overshooting curve still lands exactly on target");
	}
	
	// Setting the same target while animating is a no-op that does not disturb the animation.
	{
		auto g = std::make_shared<group>();
		animated_float value(0.0f);
		g->animate(1.0, g->elapsed(), curves::linear, [&] { value.setValue(10.0f); });
		g->update(0.0);
		g->update(0.5);
		auto midway = value.animatedValue();
		auto changed = value.setValue(10.0f);
		check(!changed && value.isAnimating() && value.animatedValue() == midway, "re-setting the same target does not restart or cancel the animation");
	}
	
	// setImmediate and setTargetValue.
	{
		auto g = std::make_shared<group>();
		animated_float value(0.0f);
		g->animate(1.0, g->elapsed(), curves::linear, [&] { value.setValue(10.0f); });
		g->update(0.0);
		g->update(0.5);
		value.setImmediate(3.0f);
		check(!value.isAnimating() && value.animatedValue() == 3.0f && value.targetValue() == 3.0f, "setImmediate cancels and snaps both value and target");
		
		// setTargetValue moves the target but leaves a settled value alone.
		value.setTargetValue(7.0f);
		check(value.targetValue() == 7.0f && value.animatedValue() == 7.0f, "setTargetValue moves a settled value with the target");
	}
	
	// A callback that destroys an animating value mid-tick cancels cleanly (no crash, no leak).
	{
		auto g = std::make_shared<group>();
		auto victim = std::make_unique<animated_float>(0.0f);
		// The destroyer is scheduled first, so it runs and destroys the victim before the tick
		// reaches the victim's still-pending animator in its snapshot.
		g->addAnimator(0.0, 0.0, [&](double, double) { victim.reset(); });
		g->animate(1.0, g->elapsed(), curves::linear, [&] { victim->setValue(10.0f); });
		g->update(0.0);
		check(!victim, "a value destroyed from within a tick is gone");
		check(g->empty(), "destroying a value mid-tick cancels its pending animator");
	}
}
	
} // namespace
