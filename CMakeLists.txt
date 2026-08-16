#----------------------------------------
# AnimationLib
#
# Header-only C++23 animation system: retained animation groups that advance a set of timed
# callbacks each frame, a fluent chain builder for sequential (.then) and parallel (.with)
# composition, first-class easing curves (named presets plus cubic-bezier and closed-form
# spring factories), and animated_value<T> — a value that transitions smoothly when set
# inside an animation block. A single empty anchor source (src/AnimationLib.cpp) is compiled
# so the header-only library still produces a static archive.
#----------------------------------------

cmake_minimum_required(VERSION 3.20)

# Declare a project only at top level; when added via add_subdirectory() the
# parent project() and toolchain govern. The version is what find_package(AnimationLib 1.2)
# negotiates against; it is carried here rather than in a tag so a source checkout and an
# installed package agree without consulting git.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	project(AnimationLib VERSION 1.0.0 LANGUAGES CXX)
endif()

# GNUInstallDirs defines CMAKE_INSTALL_INCLUDEDIR / _LIBDIR, which the public include
# directories below reference through their install-tree generator expression.
include(GNUInstallDirs)

#----------------------------------------
# Primary build
#
# True when this library owns its install rules and test targets: either it is the top-level
# project, or the repo-root superbuild has declared it a deliberate member of a multi-library
# build rather than somebody's private subdirectory. A sibling that pulls this library in with
# add_subdirectory gets neither, which is what keeps its headers out of the wrong install prefix.
#----------------------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR OR WW_SUPERBUILD)
	set(ANIMATIONLIB_PRIMARY_BUILD ON)
else()
	set(ANIMATIONLIB_PRIMARY_BUILD OFF)
endif()

add_library(AnimationLib)

#----------------------------------------
# Namespaced alias
#
# Consumers spell the dependency AnimationLib::AnimationLib whether they picked it up through
# add_subdirectory() or find_package(); the double colon also makes CMake fail at configure
# time on a misspelling rather than passing the name through to the linker as a bare -l.
#----------------------------------------
add_library(AnimationLib::AnimationLib ALIAS AnimationLib)

#----------------------------------------
# Public API
#
# Headers live under include/AnimationLib/, so the include path is `include` and consumers keep
# writing  #include "AnimationLib/Animation.h". The library is pure standard C++23 with no
# required first-party dependencies — a consumer needs nothing but this include directory.
# (Value.h additionally offers a valuesEqual specialization for MathLib vector types, guarded by
# __has_include, when MathLib happens to be on the include path; MathLib is never required.)
#
# The two generator expressions describe the same headers in the two places they can live: the
# source tree while AnimationLib is being built, and the install prefix once it has been packaged.
# An exported target may not name a path inside the source directory, so a plain `include`
# here would make install(EXPORT) below fail outright.
#----------------------------------------
target_include_directories(AnimationLib PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_features(AnimationLib PUBLIC cxx_std_23)

# Header-only implementation; this empty anchor TU only exists to produce an archive.
target_sources(AnimationLib PRIVATE
	src/AnimationLib.cpp)

#----------------------------------------
# Optional SelfTest component
#
# AnimationLib/SelfTest.h is the one header that reaches outside AnimationLib: it includes
# "CoreLib/SelfTestRegistry.h" so the inline module self-tests register with CoreLib's runner.
# No other AnimationLib header names CoreLib, so CoreLib is not a dependency of the library —
# it is an opt-in add-on, and this is how a consumer opts in:
#
#   find_package(AnimationLib COMPONENTS SelfTest)
#   target_link_libraries(my_tests PRIVATE AnimationLib::SelfTest)
#
# Deliberately not part of AnimationLibTargets: an exported target may not link a target CMake
# cannot resolve at export time, and the two worlds reach CoreLib differently. Here it is a
# sibling checkout under the repo root; for an installed consumer AnimationLibConfig.cmake builds
# the equivalent target out of the real CoreLib::CoreLib package. Both spell the same name.
#----------------------------------------
get_filename_component(WW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
set(CORELIB_INCLUDE_DIR "${WW_REPO_ROOT}/CoreLib/include"
	CACHE PATH "Directory containing the CoreLib/ header tree (used by the SelfTest component)")

add_library(AnimationLib_SelfTest INTERFACE)
add_library(AnimationLib::SelfTest ALIAS AnimationLib_SelfTest)
target_link_libraries(AnimationLib_SelfTest INTERFACE AnimationLib)
target_include_directories(AnimationLib_SelfTest INTERFACE "${CORELIB_INCLUDE_DIR}")

#----------------------------------------
# Install the static archive to lib/ and the public headers to include/.
#
# Only when AnimationLib is the top-level build. An add_subdirectory() consumer — e.g. a
# sibling library's test build that links AnimationLib — must not inherit these rules, or
# AnimationLib's headers install into that library's prefix and, from there, into its packaged
# xcframework, colliding with AnimationLib.xcframework when the app links both ("Multiple
# commands produce .../include/AnimationLib/*.h"). Matches the top-level guard on the test
# target below.
#----------------------------------------
if(ANIMATIONLIB_PRIMARY_BUILD)
	include(CMakePackageConfigHelpers)

	set(ANIMATIONLIB_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/AnimationLib")

	install(TARGETS AnimationLib
		EXPORT AnimationLibTargets
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

	# Headers only. The include tree also carries .DS_Store files, which would otherwise be packaged
	# into the xcframework's Headers directory and shipped. An allow-list rather than a
	# PATTERN ... EXCLUDE blacklist, so no new kind of stray file can leak in later.
	install(DIRECTORY include/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		FILES_MATCHING PATTERN "*.h")

	install(EXPORT AnimationLibTargets
		FILE AnimationLibTargets.cmake
		NAMESPACE AnimationLib::
		DESTINATION "${ANIMATIONLIB_INSTALL_CMAKEDIR}")

	# configure_package_config_file (rather than a plain configure_file) is what makes the installed
	# package relocatable: it resolves paths relative to the config file's own location, so the prefix
	# may be moved, mounted, or unpacked anywhere after the fact.
	configure_package_config_file(
		cmake/AnimationLibConfig.cmake.in
		"${CMAKE_CURRENT_BINARY_DIR}/AnimationLibConfig.cmake"
		INSTALL_DESTINATION "${ANIMATIONLIB_INSTALL_CMAKEDIR}")

	# SameMajorVersion: the public headers carry the ABI, so a consumer pinned to 1.x accepts any
	# later 1.y but never a 2.0 that may have moved something out from under it.
	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/AnimationLibConfigVersion.cmake"
		VERSION ${PROJECT_VERSION}
		COMPATIBILITY SameMajorVersion)

	install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/AnimationLibConfig.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/AnimationLibConfigVersion.cmake"
		DESTINATION "${ANIMATIONLIB_INSTALL_CMAKEDIR}")
endif()

#----------------------------------------
# Tests
#
# The library itself is pure standard C++23, but its self-tests register with CoreLib's self-test
# registry so they run alongside the rest of the wobbleworks suite. Each module ships its
# self-test inline in its header (asserting through AnimationLib's own SelfTestCheck.h); the
# opt-in SelfTest.h aggregator and the standalone runner under test/ register them through
# CoreLib's registry and map the failure count to the process exit code a continuous-integration
# job needs. CoreLib is therefore a TEST-ONLY dependency — the shipped library never includes it.
# Built and registered with CTest whenever AnimationLib is the top-level project and testing is
# enabled (build_lib passes -DBUILD_TESTING=OFF when assembling the xcframework). DEBUG is defined
# so the registry (CoreLib/SelfTestRegistry.h) is active. The tests drive groups with synthetic
# times, so they run identically on every platform.
#----------------------------------------
if(ANIMATIONLIB_PRIMARY_BUILD)
	set(ANIMATIONLIB_IS_TOP_LEVEL ON)
else()
	set(ANIMATIONLIB_IS_TOP_LEVEL OFF)
endif()

if(ANIMATIONLIB_IS_TOP_LEVEL AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
	enable_testing()

	# Links the SelfTest component rather than AnimationLib plus a hand-carried CoreLib include
	# path: the runner is the archetypal consumer of that component, so wiring it this way makes
	# the self-test double as the component's own verification.
	add_executable(animationlib_selftest test/animationlib_selftest_main.cpp)
	target_link_libraries(animationlib_selftest PRIVATE AnimationLib::SelfTest)
	target_compile_definitions(animationlib_selftest PRIVATE DEBUG=1)

	add_test(NAME AnimationLibSelfTest COMMAND animationlib_selftest)
endif()
