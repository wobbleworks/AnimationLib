# AnimationLib

A small **C++23 animation system**: animation groups that advance a set of timed
callbacks each frame, a fluent builder for sequencing and parallelizing them,
first-class easing curves, and values that animate themselves when you set them
inside an animation block. The implementation is header-only and **pure standard
C++23** — it has no required dependencies, and the same headers build on Clang,
GCC, and MSVC. (CoreLib is used only to run the self-tests; see
[Building and testing](#building-and-testing).)

## Model

An animation is just a callback — `void(double elapsed, double duration)` — held
in a **group** with a delay, a duration, and a tag. Each frame the host calls
`group::update(time)`; the group advances every callback, clamping `elapsed` to
`duration` and removing each one as it completes. There is no per-animation
object hierarchy: composition happens by scheduling callbacks at computed times.

```cpp
#include "AnimationLib/Animation.h"   // umbrella: group + chain + curves + free animate()

auto group = Animation::group::shared();   // the process-wide group the render loop ticks
group->update(now);                         // once per frame
```

## Curves

An easing curve is a plain callable, `Animation::Curve = std::function<double(double)>`,
evaluated for `t` in `[0, 1]`. Named presets are constants; parametric curves are
factories. Because a curve is just a callable, there is no interpolation enum and
no runtime dispatch table.

```cpp
#include "AnimationLib/Curve.h"

Animation::curves::linear;      // identity
Animation::curves::easeIn;      // accelerate
Animation::curves::easeOut;     // decelerate
Animation::curves::easeInOut;   // both
Animation::curves::step;        // snap at the midpoint
Animation::curves::springIn;    // overshoot past 1, then settle

Animation::curves::cubicBezier(0.25, 0.1, 0.25, 1.0);  // CSS cubic-bezier() timing
Animation::curves::spring(200, 12);                    // closed-form underdamped spring
```

A curve's output may exceed `[0, 1]` on purpose — `springIn`, `spring`, and some
beziers overshoot before settling at 1.

## Animated values

`Animation::animated_value<T>` (aliases `animated_float`, `animated_double`, and a
`toggle<T>` on/off wrapper) holds a value that transitions smoothly when set
inside an animation block. Set it plainly outside a block and it snaps; set it
inside `animate(...)` and it interpolates from its current value to the target.

```cpp
#include "AnimationLib/Value.h"
#include "AnimationLib/Toggle.h"

Animation::animated_float opacity = 0;

Animation::animate(0.3, [&] {
    opacity.setValue(1.0f, true, Animation::curves::easeOut);
});

// ... each frame, after group::update():
float shown = opacity.animatedValue();   // the in-flight value
```

The value's in-flight state lives in a shared block the animator captures by
`shared_ptr`, so a callback in flight during a tick stays safe even if the owning
value is destroyed on another thread meanwhile.

## Sequencing

`group::animate(...)` and `addAnimator(...)` return a `chain`, a statement-scoped
builder for composing animations sequentially (`.then`) and in parallel (`.with`):

```cpp
#include "AnimationLib/Chain.h"

group->addAnimator(0.5, fadeIn)
     .then(0.5, moveUp)
     .with(0.5, growLabel)
     .thenAfterDelay(0.2, settle);
```

## Building and testing

To **use** the library, put `include/` on your header search path and include what
you need — it is pure standard C++23, so nothing else is required.

It builds with CMake as a static archive (one empty anchor TU) so it packages like
the other wobbleworks libraries. The **self-tests** are the only part that reaches
outside the standard library: they live inline in each module header (asserting
through AnimationLib's own `SelfTestCheck.h`) and register through
[CoreLib](https://github.com/wobbleworks/CoreLib)'s self-test registry, so CoreLib
is checked out alongside AnimationLib in CI and used only by the test runner — never
by the shipped library.

```sh
cmake -S AnimationLib -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

The self-tests drive groups with synthetic times (no wall clock), so they behave
identically on every platform, and CI runs them under AddressSanitizer +
UndefinedBehaviorSanitizer (with LeakSanitizer on Linux) to guard the memory and
lifetime contracts.

## License

Apache License 2.0 — see [LICENSE](LICENSE) and [NOTICE](NOTICE).
