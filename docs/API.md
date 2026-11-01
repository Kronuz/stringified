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

log_line(stringified(owned));             // explicit; view aliases owned's buffer
log_line(view);                           // implicit; c_str() copies once
log_line(stringified("from a literal"));  // explicit; c_str() aliases the literal
log_line(stringified(std::string("tmp"))); // rvalue moves into the backing store
```

The implicit `operator std::string_view()` means a `stringified` drops straight
into anything that takes a `std::string_view`. Reach for `c_str()` only at the
boundary where a `const char*` is required.

## The interface

- **Construct from** `std::string` (lvalue or rvalue, explicit), a `const char*`
  or `char*` such as a string literal or a char buffer (explicit), or a
  `std::string_view` (implicit). A literal is never an implicit argument, so
  `f(stringified)` next to `f(std::string_view)` or `f(const std::string&)` stays
  unambiguous.
- **Read as a view:** `size()` / `length()`, `empty()`, `data()`, `operator[]`,
  `at()`, `front()`, `back()`, `begin()`/`end()` (and the `c` variants), and an
  implicit conversion to `std::string_view`.
- **`c_str()`:** returns a null-terminated `const char*`. If the wrapped view is
  known to be terminated (a whole `std::string` or a `const char*`), it points
  back into that buffer with no copy. A `string_view` is copied into the backing
  store once, on the first call.
- **`operator<<`:** writes the wrapped contents to a `std::ostream`.
