# Changelog

All notable changes to this library. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). The library has no
release tags yet, so entries are dated.

Upgrading a pin from before 2026-10-30: [the consumer upgrade note](https://github.com/Kronuz/cpp-libraries/blob/main/docs/UPGRADING-2026-10.md#stringified) lists what changes for callers, how to find affected inputs, the commits not to pin, and the speedup.

## 2026-10-31

### Added

- An explicit `stringified(const char*)` / `stringified(char*)` constructor. A C
  string, literal or char buffer is null-terminated by contract, so `c_str()`
  returns it with no copy. Literals bound to the `string_view` constructor
  before, and so paid one copy on their first `c_str()`.
- `test/overload_test.cc` (CTest `stringified_overloads`): overload sets over
  `stringified` and `std::string_view` / `const std::string&` called with a
  literal, a `const char*` and a char buffer, `static_assert`s on which
  conversions are implicit, and a type convertible to both `const char*` and
  `std::string_view`. It fails to build when either ambiguity returns.

### Fixed

- **The first version of the `const char*` constructor (commit `2c78a1d`) was
  implicit and broke overloads.** `f("abc")` was ambiguous between `f(stringified)`
  and `f(std::string_view)`, and between `g(stringified)` and
  `g(const std::string&)`, because a literal then had two different user-defined
  conversions; before it, only the other overload was viable. A
  `stringified(static_string)` was also ambiguous, since `static_string` converts
  to both `const char*` and `std::string_view`. The constructor is now explicit
  and a template on the exact types `const char*` and `char*`, so a literal never
  converts implicitly and a `static_string` takes the `std::string_view`
  constructor. Direct initialization (`stringified(path)`) is unchanged, so fs,
  storage and Xapiand compile as before. Code that passed a literal to a
  parameter of type `stringified` against `2c78a1d` must now write
  `stringified("...")`; it did not compile before `2c78a1d` either. A constructor
  template alone does not remove the first ambiguity, since two different
  constructors are never ranked.
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
