///----------------------------------------
/// @file Curve.h
/// @ingroup AnimationLib
/// @brief Easing curves as first-class callable values for the modern animation API.
/// @details A Curve maps normalized progress t in [0, 1] to a shaped value. Named
///          curves are provided as constants; parametric curves (cubic bezier and a
///          closed-form spring) are produced by factory functions. Because a curve is
///          just a callable, there is no interpolation enum and no runtime dispatch
///          table to maintain.
/// @author Created by John Stephen on 6/2/26.
/// @copyright Copyright © 2026 wobbleworks.com. All rights reserved.
///----------------------------------------

#pragma once

#include "AnimationLib/SelfTestCheck.h"

#include <functional>
#include <algorithm>
#include <cmath>
#include <numbers>

///----------------------------------------
namespace Animation {
///----------------------------------------

///----------------------------------------
/// @brief A shaping function mapping normalized progress t in [0, 1] to an eased value.
/// @details A curve is only ever evaluated for t in [0, 1]: the animation engine clamps
///          elapsed time to duration before mapping, so a curve never sees t outside that
///          range and its body need not guard against it. Manual callers are expected to
///          clamp at the call site. The @e output is unconstrained — overshooting curves
///          (springIn, spring, some cubic beziers) intentionally exceed [0, 1] before
///          settling at 1.
///----------------------------------------

using Curve = std::function<double(double)>;

///----------------------------------------
/// @brief Linearly interpolate between two values by a (typically eased) factor.
/// @details Free-function customization point: overload Animation::lerp for vector or
///          color types whose scalar is not double (e.g. simd_float3) so the factor
///          multiplies in the element type.
/// @tparam T Value type supporting addition and scalar multiplication.
/// @param from Start value.
/// @param to End value.
/// @param t Interpolation factor, normally in [0, 1].
/// @return The interpolated value.
///----------------------------------------

template <class T>
[[nodiscard]] constexpr T lerp(const T& from, const T& to, double t) {
	return from + (to - from) * t;
}

///----------------------------------------
/// MARK: Curves
///----------------------------------------

///----------------------------------------
namespace curves {
///----------------------------------------

/// @brief Identity curve; progress passes through unchanged.
inline const Curve linear = [](double t) {
	return t;
};

/// @brief Step curve; snaps from 0 to 1 at the midpoint.
inline const Curve step = [](double t) {
	return t < 0.5 ? 0.0 : 1.0;
};

/// @brief Decelerating curve (fast start, slow end).
inline const Curve easeOut = [](double t) {
	constexpr auto half_pi = std::numbers::pi / 2;
	return std::sin(t * half_pi);
};

/// @brief Accelerating curve (slow start, fast end).
inline const Curve easeIn = [](double t) {
	constexpr auto half_pi = std::numbers::pi / 2;
	return 1 - std::cos(t * half_pi);
};

/// @brief Accelerate then decelerate (slow at both ends).
inline const Curve easeInOut = [](double t) {
	return 0.5 * (1 - std::cos(t * std::numbers::pi));
};

/// @brief Overshoot curve: rises past 1 (peak ≈1.155 at t = 0.75) then returns to 1.
/// @details Evaluates sin(t·2π/3) / sin(2π/3), the shape used by terrain-style transitions.
inline const Curve springIn = [](double t) {
	constexpr auto endAngle = 2.0 * std::numbers::pi / 3.0;
	return std::sin(t * endAngle) / std::sin(endAngle);
};

///----------------------------------------
/// @brief Build an easing curve from a cubic bezier with endpoints (0,0) and (1,1).
/// @details The two control points (x1,y1) and (x2,y2) shape the curve, matching the
///          CSS cubic-bezier() timing model. The parameter is solved for the bezier x
///          coordinate via bisection, then the matching y is returned.
/// @param x1 First control point x.
/// @param y1 First control point y.
/// @param x2 Second control point x.
/// @param y2 Second control point y.
/// @return A Curve evaluating the bezier shape.
///----------------------------------------

[[nodiscard]] inline Curve cubicBezier(double x1, double y1, double x2, double y2) {
	return [=](double t) {
		// Cubic bezier basis for P0=(0,0), P3=(1,1)
		auto sampleX = [=](double u) {
			auto v = 1 - u;
			return 3 * v * v * u * x1 + 3 * v * u * u * x2 + u * u * u;
		};
		
		auto sampleY = [=](double u) {
			auto v = 1 - u;
			return 3 * v * v * u * y1 + 3 * v * u * u * y2 + u * u * u;
		};
		
		// Solve sampleX(u) == t for the bezier parameter u by bisection
		auto low = 0.0;
		auto high = 1.0;
		auto u = t;
		
		for (auto i = 0; i < 24; ++i) {
			auto x = sampleX(u);
			if (std::abs(x - t) < 1e-6) {
				break;
			}
			
			if (x < t) {
				low = u;
			} else {
				high = u;
			}
			
			u = 0.5 * (low + high);
		}
		
		// Return the shaped value at the solved parameter
		return sampleY(u);
	};
}

///----------------------------------------
/// @brief Build a closed-form underdamped spring curve.
/// @details Models the step response of a unit-mass spring, normalized so that t=0 maps
///          to 0 and t=1 is essentially settled at 1, with overshoot determined by the
///          damping. The damping ratio is clamped below critical, so the curve always
///          oscillates; for a non-overshooting response use easeOut or a cubic bezier.
/// @param stiffness Spring stiffness (higher is snappier).
/// @param damping Damping coefficient (lower bounces more).
/// @return A Curve evaluating the spring shape.
///----------------------------------------

[[nodiscard]] inline Curve spring(double stiffness, double damping) {
	// Derive the natural frequency and a sub-critical damping ratio
	auto omega = std::sqrt(stiffness);
	auto zeta = std::clamp(damping / (2 * omega), 0.0001, 0.9999);
	auto omegaDamped = omega * std::sqrt(1 - zeta * zeta);
	
	// Normalize so t=1 reaches ~0.1% of the settling envelope
	auto settleTime = -std::log(0.001) / (zeta * omega);
	
	return [=](double t) {
		// Evaluate the underdamped step response at the scaled time
		auto tau = t * settleTime;
		auto envelope = std::exp(-zeta * omega * tau);
		return 1 - envelope * (std::cos(omegaDamped * tau)
			+ (zeta * omega / omegaDamped) * std::sin(omegaDamped * tau));
	};
}

///----------------------------------------
} // namespace curves
///----------------------------------------

///----------------------------------------
/// MARK: Self-test
///----------------------------------------

///----------------------------------------
/// @brief Exercises the easing curves and the parametric curve factories.
/// @details Checks endpoint identities for every named curve, validates springIn and
///          easeInOut against their closed-form definitions, confirms the cubic bezier
///          collapses to the identity when its control points sit on the diagonal
///          (a differential check of the x-parameter solver against lerp), verifies a
///          general bezier is monotone and bounded, and confirms the spring starts at 0,
///          settles at 1, and overshoots only when underdamped.
///----------------------------------------

inline void curveSelfTest() {
	using Animation::selftest::check;
	constexpr double tolerance = 1.0e-6;
	
	// Endpoint identities: every named curve pins 0 and 1.
	check(std::abs(curves::linear(0.0)) < tolerance && std::abs(curves::linear(1.0) - 1.0) < tolerance, "linear pins its endpoints");
	check(curves::step(0.0) == 0.0 && curves::step(1.0) == 1.0, "step pins its endpoints");
	check(std::abs(curves::easeIn(0.0)) < tolerance && std::abs(curves::easeIn(1.0) - 1.0) < tolerance, "easeIn pins its endpoints");
	check(std::abs(curves::easeOut(0.0)) < tolerance && std::abs(curves::easeOut(1.0) - 1.0) < tolerance, "easeOut pins its endpoints");
	check(std::abs(curves::easeInOut(0.0)) < tolerance && std::abs(curves::easeInOut(1.0) - 1.0) < tolerance, "easeInOut pins its endpoints");
	check(std::abs(curves::springIn(0.0)) < tolerance && std::abs(curves::springIn(1.0) - 1.0) < tolerance, "springIn pins its endpoints");
	
	// Closed-form parity across the range.
	for (auto step = 0; step <= 20; ++step) {
		auto t = step / 20.0;
		auto endAngle = 2.0 * std::numbers::pi / 3.0;
		check(std::abs(curves::springIn(t) - std::sin(t * endAngle) / std::sin(endAngle)) < tolerance, "springIn matches its formula");
		check(std::abs(curves::easeInOut(t) - 0.5 * (1 - std::cos(t * std::numbers::pi))) < tolerance, "easeInOut matches its formula");
	}
	
	// springIn peaks at t = 0.75 with the hand-derived height 1/sin(120°) — an independent
	// reference, not a re-transcription of the implementation. This is the overshoot the
	// terrain transition relies on.
	check(std::abs(curves::springIn(0.75) - 1.0 / std::sin(2.0 * std::numbers::pi / 3.0)) < tolerance, "springIn peaks at the derived height");
	check(curves::springIn(0.75) > curves::springIn(0.70) && curves::springIn(0.75) > curves::springIn(0.80), "springIn's peak is at t = 0.75");
	
	// A cubic bezier with control points on the diagonal is the identity — a differential
	// check of the bisection x-solver against a plain linear interpolation.
	auto diagonal = curves::cubicBezier(1.0 / 3.0, 1.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0);
	for (auto step = 0; step <= 20; ++step) {
		auto t = step / 20.0;
		check(std::abs(diagonal(t) - t) < 1.0e-5, "diagonal cubic bezier reproduces the identity");
	}
	
	// The CSS "ease" bezier is monotone non-decreasing and stays within [-ε, 1+ε].
	auto ease = curves::cubicBezier(0.25, 0.1, 0.25, 1.0);
	auto previous = ease(0.0);
	for (auto step = 0; step <= 100; ++step) {
		auto value = ease(step / 100.0);
		check(value >= previous - 1.0e-6, "the ease bezier is monotone non-decreasing");
		check(value >= -1.0e-6 && value <= 1.0 + 1.0e-6, "the ease bezier stays bounded");
		previous = value;
	}
	
	// A spring starts at 0, settles near 1 by t = 1, and overshoots only when underdamped.
	auto bouncy = curves::spring(200, 10);
	check(std::abs(bouncy(0.0)) < tolerance, "the spring starts at 0");
	check(std::abs(bouncy(1.0) - 1.0) < 0.05, "the spring settles near 1");
	auto bouncyMax = 0.0;
	auto stiffMax = 0.0;
	auto stiff = curves::spring(200, 28);
	for (auto step = 0; step <= 200; ++step) {
		bouncyMax = std::max(bouncyMax, bouncy(step / 200.0));
		stiffMax = std::max(stiffMax, stiff(step / 200.0));
	}
	check(bouncyMax > 1.0, "an underdamped spring overshoots");
	check(stiffMax < 1.05, "a near-critically-damped spring barely overshoots");
}

///----------------------------------------
} // namespace Animation
///----------------------------------------
