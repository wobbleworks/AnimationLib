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
# parent project() and toolchain govern.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	project(AnimationLib CXX)
endif()

add_library(AnimationLib)

#----------------------------------------
# Public API
#
# Headers live under include/AnimationLib/, so the include path is `include` and consumers keep
# writing  #include "AnimationLib/Animation.h". The library is pure standard C++23 with no
# required first-party dependencies — a consumer needs nothing but this include directory.
# (Value.h additionally offers a valuesEqual specialization for MathLib vector types, guarded by
# __has_include, when MathLib happens to be on the include path; MathLib is never required.)
#----------------------------------------
target_include_directories(AnimationLib PUBLIC include)

target_compile_features(AnimationLib PUBLIC cxx_std_23)

# Header-only implementation; this empty anchor TU only exists to produce an archive.
target_sources(AnimationLib PRIVATE
	src/AnimationLib.cpp)

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
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	include(GNUInstallDirs)

	install(TARGETS AnimationLib
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

	install(DIRECTORY include/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
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
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(ANIMATIONLIB_IS_TOP_LEVEL ON)
else()
	set(ANIMATIONLIB_IS_TOP_LEVEL OFF)
endif()

if(ANIMATIONLIB_IS_TOP_LEVEL AND (NOT DEFINED BUILD_TESTING OR BUILD_TESTING))
	enable_testing()

	# The self-test runner reaches CoreLib's registry (SelfTest.h -> CoreLib/SelfTestRegistry.h);
	# CoreLib is a sibling repository under the repo root, needed only by this test target.
	get_filename_component(WW_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

	add_executable(animationlib_selftest test/animationlib_selftest_main.cpp)
	target_link_libraries(animationlib_selftest PRIVATE AnimationLib)
	target_include_directories(animationlib_selftest PRIVATE "${WW_REPO_ROOT}/CoreLib/include")
	target_compile_definitions(animationlib_selftest PRIVATE DEBUG=1)

	add_test(NAME AnimationLibSelfTest COMMAND animationlib_selftest)
endif()
