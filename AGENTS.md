# AGENTS.md

Working notes for agents modifying this repository. For the design read
`ARCHITECTURE.md`; for usage read `README.md`. This file covers the repo layout,
how to build and test, the invariants you must not break, and the traps that are
easy to fall into.

## Repo map

```
stringified.hh               std::string_view wrapper with a backing std::string for null-terminated c_str(). Header. Verbatim from Xapiand.
test/test.cc                 Runnable smoke test: view accessors, construction from each input, c_str() termination, iterators, operator<<.
CMakeLists.txt               INTERFACE library `stringified` (+ alias stringified::stringified); CTest test `stringified`.
LICENSE                      MIT, Copyright (c) 2015-2018 Dubalu LLC.
README.md                    What it is, install, quick start.
docs/API.md                  Usage and the full interface.
ARCHITECTURE.md              Internal design: the two members, construction, the c_str() contract.
```

Everything is header-only. There is no `.cc` to compile except the test. The
CMake target is a pure `INTERFACE` library that only adds the source dir to the
include path and requests `cxx_std_20`.

## Build and run the test

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

Expected output ends with `all stringified tests passed`, exit 0. The test
target is `stringified_test`; the registered CTest name is `stringified`.

## Conventions

- **C++20.** The target requests `cxx_std_20` to stay uniform with the sibling
  libraries; the header itself is older-standard-friendly, but don't drop the
  target below it.
- **Zero external dependencies.** The only includes are `<string>`,
  `<string_view>`, and `<ostream>`. Don't add anything else.
- **Filename is stable.** The header keeps its original Xapiand name
  (`stringified.hh`) so a consumer that already `#include`s it just needs this
  repo on the include path. Don't rename it.
- Tabs for indentation in the test, double quotes in code, no em dashes in
  prose. (The header keeps its original spacing from Xapiand.)

## Load-bearing invariants

- **`_str_view` is the source of truth for every read.** All accessors
  (`size`/`length`, `empty`, `data`, indexing, iterators, the `std::string_view`
  conversion, `operator<<`) read `_str_view`, never `_str` directly. `_str` is
  only ever the backing store. Keep new accessors reading the view.
- **`c_str()` never reads past the view.** It returns the view's data unchanged
  only when `_terminated` is set (the view is a whole `std::string` the caller
  owns, or our own `_str`); otherwise it copies into `_str` and re-points the
  view. A bare `string_view` is never assumed terminated, so a literal wrapped
  as a `string_view` is copied on the first `c_str()`. Wrap a `std::string`
  to avoid that copy.
- **The members are `mutable` on purpose.** `c_str()` is `const` but may fill
  `_str` and re-point `_str_view`. Keep both members `mutable`.
- **Construction ownership matters.** `std::string&&` moves into `_str` (the
  wrapper owns it); `const std::string&` and the `string_view` ctors alias the
  caller's data. Don't change which constructors copy.

## Traps

- **Copy and move re-point the view.** A view into the source's own `_str` is
  rebound to the destination's `_str` (a short string's buffer lives inside the
  object, so copying the view would dangle); a view over caller storage is
  copied as is. Older versions copied the view verbatim (dangling) and
  `c_str()` peeked at the byte past a `string_view`; both were bugs, fixed.
- **It's a view first.** Prefer the implicit `std::string_view` conversion for
  reads; reach for `c_str()` only at a `const char*` boundary, since that's the
  only path that can trigger a copy.

## How to extend

- **Add a read accessor.** Mirror the existing ones: forward to `_str_view`,
  mark it `const`, keep it a thin pass-through. Don't touch `_str`.
- **Always extend the smoke test.** `test/test.cc` is the only executable check.
  It covers construction from each input type, the no-copy and copy paths of
  `c_str()`, the iterators, and `operator<<`. Keep both `c_str()` paths covered
  when you change anything near it.

## Standalone vs. Xapiand

This is a standalone extraction from
[Xapiand](https://github.com/Kronuz/Xapiand). `stringified.hh` had zero
dependencies already (only the standard `<string>`/`<string_view>`/`<ostream>`
headers), so it was copied verbatim, there is no decoupling delta. Keep it that
way; any change here should be reconcilable with upstream as a plain edit.
