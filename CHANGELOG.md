# Changelog

All notable changes to this library. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). The library has no
release tags yet, so entries are dated.

## 2026-10-31

### Added

- A `stringified(const char*)` constructor. A C string, literal or char buffer is
  null-terminated by contract, so `c_str()` returns it with no copy. Literals
  bound to the `string_view` constructor before, and so paid one copy on their
  first `c_str()`.

### Fixed

- **Copy and move kept a dangling view.** Copy and move construction and
  assignment copied the `string_view` verbatim, so a view into the source's
  short-string buffer (or heap block) dangled once the source died or was moved
  from. The view is rebound to the destination's `_str` when the source's view
  pointed at the source's `_str`.
- **`c_str()` read past a `string_view`.** It read the byte after the view to
  decide whether it was already terminated, which is out of bounds for a view
  over an exact-size buffer. `c_str()` relies on an explicit terminated flag (a
  whole `std::string`, a C string, or its own storage) and otherwise copies once.
- The `std::format` support is guarded by `__cpp_lib_format`.

Oracle: the `std::string_view` and `std::string` contracts and AddressSanitizer.
The tests copy, move and assign sizes 0 to 80 and an exact-size heap view; they
fail on the old header (heap-use-after-free) and pass under ASan and UBSan with
GCC 11 and clang 12.

## 2026-10-30

### Fixed

- `stringified(std::string&&)` initialized `_str` from the named rvalue
  reference, an lvalue, so it copied the string. It moves it, so a
  heap-allocated string keeps its buffer. A regression test checks that `data()`
  is unchanged.
