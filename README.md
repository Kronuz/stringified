# stringified

A small, header-only, dependency-free **string_view wrapper** for C++20,
extracted from [Xapiand](https://github.com/Kronuz/Xapiand).

## What it is

One header, `stringified.hh`: a thin wrapper around `std::string_view` that lets
a function accept any string-shaped input (a `std::string`, a
`std::string_view`, a string literal) through one type, and hands back a
null-terminated `c_str()` when the caller needs one. It only copies the data
into its own backing store when that's actually required to guarantee
termination. The rest of the time it's a plain view over the caller's buffer.

The point is to bridge the gap between `std::string_view` (cheap, but not
null-terminated) and C APIs that want a `const char*`. You take a
`const stringified&` parameter, and the caller can pass whatever they have.
Inside, you read it as a view, or call `c_str()` when you need to hand a
`const char*` to a C function:

- **Construct from** `std::string` (lvalue or rvalue), `std::string_view`, or a
  string literal.
- **Read as a view:** `size()` / `length()`, `empty()`, `data()`, `operator[]`,
  `at()`, `front()`, `back()`, `begin()`/`end()` (and the `c` variants), and an
  implicit conversion to `std::string_view`.
- **`c_str()`:** returns a null-terminated `const char*`. If the wrapped view is
  already terminated (e.g. it spans a whole `std::string`), it points back into
  that buffer with no copy; otherwise it copies into the backing store once and
  returns that.
- **`operator<<`:** writes the wrapped contents to a `std::ostream`.

## Install

CMake with `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
  stringified
  GIT_REPOSITORY https://github.com/Kronuz/stringified.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(stringified)

target_link_libraries(your_target PRIVATE stringified::stringified)
```

The `stringified` target is a pure `INTERFACE` library: it compiles nothing,
requests `cxx_std_20`, and puts the header directory on your include path. Then:

```cpp
#include "stringified.hh"
```

Requires C++20. On macOS it builds with AppleClang/libc++, the same toolchain
Xapiand uses. The header keeps its original filename, so a codebase that already
`#include "stringified.hh"` just needs this repo on its include path.

## Usage

```cpp
#include "stringified.hh"
#include <string>
#include <string_view>

// One parameter type accepts every string flavor the caller might hold.
void log_line(const stringified& s) {
    // Read it as a view for anything that doesn't need a C string:
    if (s.empty()) return;

    // Or hand a guaranteed-null-terminated pointer to a C API. No copy when
    // s already wraps a whole std::string; a single copy when it wraps a
    // non-terminated slice.
    std::fputs(s.c_str(), stderr);
}

std::string owned = "from a std::string";
std::string_view view = "from a string_view";

log_line(owned);                          // view aliases owned's buffer
log_line(view);                           // view aliases the literal
log_line("from a literal");               // binds via the string_view ctor
log_line(std::string("temporary"));       // rvalue moves into the backing store
```

The implicit `operator std::string_view()` means a `stringified` drops straight
into anything that takes a `std::string_view`. Reach for `c_str()` only at the
boundary where a `const char*` is required.

## Build & test

```sh
cmake -B build && cmake --build build && ctest --test-dir build
```

The test checks the view accessors against each accepted input type, confirms
`c_str()` is always null-terminated (and that it copies only for a
non-terminated slice, not for a whole string), round-trips the iterators, and
verifies `operator<<`. It prints `all stringified tests passed` and exits 0.

## Provenance

Extracted from [Xapiand](https://github.com/Kronuz/Xapiand).
`stringified.hh` had zero dependencies already (only the standard
`<string>`/`<string_view>`/`<ostream>` headers), so it was copied verbatim, no
decoupling delta. See [ARCHITECTURE.md](ARCHITECTURE.md) for the design and
[AGENTS.md](AGENTS.md) for the repo map and invariants.

## License

MIT, Copyright (c) 2015-2018 Dubalu LLC. See [LICENSE](LICENSE).
