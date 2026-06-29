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
README.md                    What it is, install, usage.
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
- **`c_str()` copies only when it must.** It returns the view's data unchanged
  when the view already points into `_str` or the byte past its end is `'\0'`;
  it copies into `_str` and re-points `_str_view` only for a non-terminated
  slice. Don't "simplify" this into an unconditional copy, that defeats the
  whole purpose, and don't drop the past-the-end check.
- **The members are `mutable` on purpose.** `c_str()` is `const` but may fill
  `_str` and re-point `_str_view`. Keep both members `mutable`.
- **Construction ownership matters.** `std::string&&` moves into `_str` (the
  wrapper owns it); `const std::string&` and the `string_view` ctors alias the
  caller's data. Don't change which constructors copy.

## Traps

- **`c_str()` reads the byte just past the view's end.** This is intentional and
  safe for views into `std::string` buffers and string literals (they always
  have a readable, usually `'\0'`, byte there). It is *not* safe for a view
  hand-built over a buffer with no readable byte past the end. That's the
  original Xapiand contract; callers pass views into real string storage.
- **Copy construction copies the view, not the backing store.** The copy
  constructor copies `_str_view` but not `_str`. If the source owned its data in
  `_str` (built from a `std::string&&`), the copy's view dangles once the source
  dies. This is the upstream behavior, preserved verbatim. In practice
  `stringified` is used as a short-lived `const&` parameter, not stored or
  copied across scopes; keep using it that way rather than "fixing" the copy
  ctor, which would be a behavior change to reconcile with upstream.
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
