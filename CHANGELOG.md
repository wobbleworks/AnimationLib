# Changelog

All notable changes to AnimationLib are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## Versioning

AnimationLib follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
In a version `MAJOR.MINOR.PATCH`:

- **MAJOR** changes when the public API breaks — a symbol removed or renamed, a
  signature or a documented behavior changed, a requirement tightened.
- **MINOR** changes when the API grows in a backward-compatible way.
- **PATCH** changes for backward-compatible fixes.

The version describes this library's API and nothing else. AnimationLib is
versioned independently of any application that consumes it, so its numbers move
when its own surface moves, not on somebody else's release schedule.

Every breaking change is listed under **Changed** or **Removed** in the entry
for the release that carries it; those are the ones that force a major bump.

The CMake package version file is generated with `COMPATIBILITY
SameMajorVersion`, which is exactly this contract expressed to CMake:
`find_package(AnimationLib 1.0)` accepts any 1.x — everything in the 1.x line is
compatible with 1.0 by construction — and rejects 2.x.

## [1.0.0] — 2026-08-19

First tagged release. This code has been in production use for some time; 1.0.0
is the point at which it becomes independently versioned and separately
consumable, so the entry below describes the surface rather than a diff against
a predecessor.

### Added

- Public release of the header-only C++23 animation layer: easing and
  interpolation curves, animated values, and group sequencing.
- CMake package export: `find_package(AnimationLib CONFIG)` provides
  `AnimationLib::AnimationLib`, the same imported target name an in-tree
  `add_subdirectory()` build supplies.
- Continuous integration across macOS, Linux (GCC and Clang), and Windows, with
  the sanitized legs running AddressSanitizer and UndefinedBehaviorSanitizer
  (plus LeakSanitizer on Linux) over the full self-test suite.

### Fixed

- The generated package version file read `PROJECT_VERSION`, which this library
  leaves unset whenever it is not the top-level project — including when an
  enclosing project builds it as a deliberate package member by setting the
  `WW_SUPERBUILD` option, which is the case that also turns its install rules
  on. An installed package could therefore advertise the enclosing project's
  version instead of AnimationLib's. The version is now carried in
  `ANIMATIONLIB_VERSION`, set unconditionally ahead of `project()`.

### Notes for packagers

AnimationLib is distributed as **source only**, and being header-only there is
little to ship as a binary: the static archive it builds is an anchor around one
empty translation unit, produced so the library packages like its siblings.

The shipped library is pure standard C++23 with no first-party dependencies.
CoreLib is reached only by the self-test runner, for its self-test registry, and
the package config treats it as an optional component rather than a hard
`find_dependency()` so a consumer who only wants the library is not blocked by
its absence.
