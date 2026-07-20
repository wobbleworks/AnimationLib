///----------------------------------------
/// @file SelfTest.h
/// @ingroup AnimationLib
/// @brief Registers the AnimationLib self-tests with the shared self-test registry.
/// @details Including this header in a debug build registers each module's inline self-test
///          with @ref Core::selftest::Registry so it runs as part of @ref
///          Core::selftest::runAll. The module headers assert through AnimationLib's own @ref
///          Animation::selftest::check (see SelfTestCheck.h), so a client that includes only a
///          module header pulls in the check primitive but not CoreLib's registry; this is the
///          only AnimationLib header that reaches it. Outside a DEBUG build the registrars
///          compile to no-ops.
/// @author Created by John Stephen on 7/17/26.
/// @copyright Copyright © 2026 wobbleworks.com. All rights reserved.
///----------------------------------------

#pragma once

#include "AnimationLib/Curve.h"
#include "AnimationLib/Group.h"
#include "AnimationLib/Chain.h"
#include "AnimationLib/Value.h"
#include "AnimationLib/Toggle.h"

#include "CoreLib/SelfTestRegistry.h"

///----------------------------------------
namespace Animation {
///----------------------------------------

///----------------------------------------
/// @brief Registers @ref curveSelfTest; runs once in a debug build.
///----------------------------------------

inline Core::selftest::FunctionTestRegistrar _curveSelfTest("Animation.Curve", curveSelfTest);

///----------------------------------------
/// @brief Registers @ref groupSelfTest; runs once in a debug build.
///----------------------------------------

inline Core::selftest::FunctionTestRegistrar _groupSelfTest("Animation.Group", groupSelfTest);

///----------------------------------------
/// @brief Registers @ref chainSelfTest; runs once in a debug build.
///----------------------------------------

inline Core::selftest::FunctionTestRegistrar _chainSelfTest("Animation.Chain", chainSelfTest);

///----------------------------------------
/// @brief Registers @ref animatedValueSelfTest; runs once in a debug build.
///----------------------------------------

inline Core::selftest::FunctionTestRegistrar _animatedValueSelfTest("Animation.Value", animatedValueSelfTest);

///----------------------------------------
/// @brief Registers @ref toggleSelfTest; runs once in a debug build.
///----------------------------------------

inline Core::selftest::FunctionTestRegistrar _toggleSelfTest("Animation.Toggle", toggleSelfTest);

///----------------------------------------
} // namespace Animation
///----------------------------------------
