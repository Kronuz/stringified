# stringified

Part of [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries), a collection of small, reusable C++ libraries.

[![CI](https://github.com/Kronuz/stringified/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/stringified/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Header-only](https://img.shields.io/badge/header--only-yes-green.svg)

String parameter type for C++20: one signature for views, strings, and C strings, with
a `c_str()` that copies only when needed. Take a `const stringified&`; callers pass a
`std::string_view` implicitly, or a `std::string` or C string wrapped explicitly, and
you read it as a view. `c_str()` returns the caller's own null-terminated bytes when it
has them and copies a bare view once. Reach for it in functions that hand strings on to
C APIs but should accept any string type.

## ✨ Highlights

- **One signature** for views and strings: `std::string_view` converts implicitly;
  `std::string` and C strings take an explicit `stringified(s)`, so a literal never
  makes an overload set ambiguous.
- **No copy when possible.** `c_str()` points back into a whole `std::string` or a C
  string. It copies a bare `string_view` once, on the first call, because the byte
  after a view is unknown and never read.
- **Takes ownership of a temporary.** `stringified(std::move(s))` keeps the moved-in
  `std::string` alive for as long as the wrapper.
- **Safe to copy and move.** A copy of a wrapper that owns its bytes points at its own
  copy, never at the source's buffer.
- **Reads like a `string_view`**: `size()`, `data()`, iteration, `operator[]`, and an
  implicit conversion.
- **One header, no dependencies.**

## 🚀 Quick start

```cpp
#include "stringified.hh"

#include <cstdio>
#include <string>
#include <string_view>

void log_line(const stringified& s) {
	std::fputs(s.c_str(), stdout);   // null-terminated, copied only for a string_view
}

int main() {
	std::string owned = "from a std::string\n";
	log_line(stringified(owned));                          // explicit; no copy
	log_line(std::string_view("from a view\n"));           // implicit; copied once
	log_line(stringified("from a literal\n"));             // explicit; aliases the literal
}
// prints:
// from a std::string
// from a view
// from a literal
```

## 📦 Installation

Requires a C++20 compiler and CMake 3.14 or newer; no other dependencies. CI builds
and tests it with GCC and Clang on Linux (x86_64, arm64), macOS (arm64, x86_64), and
FreeBSD, plus ASan, UBSan, and TSan builds.

```cmake
include(FetchContent)
FetchContent_Declare(stringified
  GIT_REPOSITORY https://github.com/Kronuz/stringified.git
  GIT_TAG        0053465ae99fb0f344f3c7a16dc88b4ec473ceb0)  # or main, which tracks the tip
FetchContent_MakeAvailable(stringified)

target_link_libraries(my_app PRIVATE stringified::stringified)
```

Then `#include "stringified.hh"`. Without CMake, copy `stringified.hh` onto your
include path.

## 🎯 When to use it

Functions that pass strings on to C APIs (`open`, `getenv`, `syslog`) but should
accept any string type without an overload per flavor.

Not for:

- **Owning storage.** It is a parameter type; keep your strings in `std::string`.
- **Holding a view past its source.** Built from a `std::string_view` or an lvalue
  `std::string`, it points at the caller's bytes, which must outlive it.

## 📚 Documentation

- [docs/API.md](docs/API.md): usage and the full interface.
- [ARCHITECTURE.md](ARCHITECTURE.md): when it copies, and why.
- [CHANGELOG.md](CHANGELOG.md): notable changes.
- [AGENTS.md](AGENTS.md): repository map and traps.
- [Catalog entry](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md#stringified).

## 🛠️ Building and testing

```sh
cmake -B build
cmake --build build
ctest --test-dir build
```

## 🤝 Contributing

Issues and pull requests are welcome. Read [AGENTS.md](AGENTS.md) first, and the
[catalog's AGENTS.md](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md) for
the family's rules. CI must pass on every platform.

## 📄 License

MIT; see [LICENSE](LICENSE). Extracted from [Xapiand](https://github.com/Kronuz/Xapiand).
