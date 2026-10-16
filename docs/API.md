# stringified API reference

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

## The interface

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
