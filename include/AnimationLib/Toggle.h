///----------------------------------------
/// @file Toggle.h
/// @ingroup AnimationLib
/// @brief Animated on/off toggle with configurable on and off values.
/// @author Created by John Stephen on 2/27/26.
/// @copyright Copyright © 2026 wobbleworks.com. All rights reserved.
///----------------------------------------

#pragma once

#include "AnimationLib/Value.h"
#include "AnimationLib/SelfTestCheck.h"

///----------------------------------------
namespace Animation {
///----------------------------------------

///----------------------------------------
/// @class toggle
/// @brief Wraps an animated_value with on/off semantics.
/// @tparam T Value type (typically float or double).
///----------------------------------------

///----------------------------------------
template <class T>
class toggle {
///----------------------------------------
	bool _isOn = false;
	T _onValue = T{1};
	T _offValue = T{0};
	animated_value<T> _value;
	Curve _curve;
	
	bool updateValue() {
		return _value.setValue(_isOn ? _onValue : _offValue, true, _curve);
	}
	
public:
	toggle() = default;
	
	toggle(bool isOn)
		: _isOn(isOn), _value(isOn ? _onValue : _offValue) {}
		
	toggle(T onValue, T offValue = T{0})
		: _onValue(onValue), _offValue(offValue), _value(offValue) {}
		
	///----------------------------------------
	/// @brief Set whether the toggle is on or off.
	/// @param isOn New on/off state.
	/// @return True if the value is animating.
	///----------------------------------------
	
	bool setIsOn(bool isOn) {
		if (_isOn == isOn)
			return false;
		_isOn = isOn;
		return updateValue();
	}
	
	///----------------------------------------
	/// @brief Set the value used when the toggle is on.
	/// @param onValue New on value.
	/// @return True if the value is animating.
	///----------------------------------------
	
	bool setOnValue(T onValue) {
		if (_onValue == onValue)
			return false;
		_onValue = onValue;
		return updateValue();
	}
	
	///----------------------------------------
	/// @brief Set the value used when the toggle is off.
	/// @param offValue New off value.
	/// @return True if the value is animating.
	///----------------------------------------
	
	bool setOffValue(T offValue) {
		if (_offValue == offValue)
			return false;
		_offValue = offValue;
		return updateValue();
	}
	
	///----------------------------------------
	/// @brief Set the on/off state and on value simultaneously.
	/// @param isOn New on/off state.
	/// @param onValue New on value.
	/// @return True if the value is animating.
	///----------------------------------------
	
	bool setIsOnWithValue(bool isOn, T onValue) {
		if (_isOn == isOn && _onValue == onValue)
			return false;
		_isOn = isOn;
		_onValue = onValue;
		return updateValue();
	}
	
	/// @brief Shorthand for setOnValue().
	void setValue(T onValue) { setOnValue(onValue); }
	
	///----------------------------------------
	/// @brief Set the easing curve used when the toggle animates between its values.
	/// @param curve Easing curve; an empty curve falls back to the enclosing block's curve.
	///----------------------------------------
	
	void setCurve(Curve curve) { _curve = std::move(curve); }
	
	/// @brief Whether the toggle is currently on.
	[[nodiscard]] bool isOn() const noexcept { return _isOn; }
	
	/// @brief Whether the toggle is currently animating.
	[[nodiscard]] bool isAnimating() const noexcept { return _value.isAnimating(); }
	
	/// @brief Cancel in-progress animations.
	void cancelAnimations(bool finish = false) { _value.cancelAnimations(finish); }
	
	/// @brief The current animated (in-flight) value.
	[[nodiscard]] T animatedValue() const noexcept { return _value.animatedValue(); }
	
	/// @brief The target value the animation is heading towards.
	[[nodiscard]] T targetValue() const noexcept { return _value.targetValue(); }
	
	/// @brief Implicit conversion returns the on value.
	operator T() const noexcept { return _onValue; }
	
	bool operator=(bool isOn) {
		setIsOn(isOn);
		return isOn;
	}
	
	template <typename U = T, typename = std::enable_if_t<!std::is_same_v<U, bool>>>
	toggle& operator=(T onValue) {
		setValue(onValue);
		return *this;
	}
};

using toggle_float = toggle<float>;
using toggle_double = toggle<double>;

///----------------------------------------
/// MARK: Self-test
///----------------------------------------

///----------------------------------------
/// @brief Exercises the toggle's on/off mapping and its curve pass-through.
/// @details Confirms a toggle rests at its off value and animates to its on value when
///          switched on, and that a toggle's own curve drives the same trajectory as a bare
///          animated value given that curve (a differential check of the pass-through).
///----------------------------------------

inline void toggleSelfTest() {
	using Animation::selftest::check;
	
	// on/off maps to the configured on and off values.
	{
		toggle_float t(2.0f, 0.5f);
		check(t.targetValue() == 0.5f, "a toggle rests at its off value");
		auto g = std::make_shared<group>();
		g->animate(1.0, g->elapsed(), curves::linear, [&] { t.setIsOn(true); });
		g->update(0.0);
		g->update(1.0);
		check(std::abs(t.animatedValue() - 2.0f) < 1.0e-4f, "turning a toggle on animates to its on value");
	}
	
	// setCurve drives the same trajectory as a bare animated value given the same curve.
	{
		auto toggleGroup = std::make_shared<group>();
		auto valueGroup = std::make_shared<group>();
		toggle_float t(10.0f, 0.0f);
		t.setCurve(curves::easeOut);
		animated_float value(0.0f);
		toggleGroup->animate(1.0, [&] { t.setIsOn(true); });
		valueGroup->animate(1.0, [&] { value.setValue(10.0f, true, curves::easeOut); });
		for (auto step = 0; step <= 10; ++step) {
			auto time = step / 10.0;
			toggleGroup->update(time);
			valueGroup->update(time);
			check(std::abs(t.animatedValue() - value.animatedValue()) < 1.0e-4f, "a toggle's curve matches a bare animated value");
		}
	}
}
	
} // namespace
