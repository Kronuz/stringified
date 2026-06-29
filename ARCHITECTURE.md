# Architecture

The internal design of `stringified`: a single header wrapping
`std::string_view` with a lazily-populated backing `std::string` so it can both
read as a view and hand out a null-terminated `c_str()`. For usage see
`README.md`; for the repo map and invariants see `AGENTS.md`.

## Shape

One header, header-only, nothing behind it but the standard string headers:

```
  stringified.hh   a std::string_view plus a backing std::string for c_str()
```

## The two members

A `stringified` holds two things, both `mutable`:

```
  std::string      _str        backing store, populated only when needed
  std::string_view _str_view   the live view every accessor reads
```

`_str_view` is the source of truth for every read accessor (`size`, `empty`,
`operator[]`, the iterators, the implicit `std::string_view` conversion, and so
on). `_str` exists only to back a null-terminated `c_str()` when the wrapped
view doesn't already sit in a terminated buffer. They're `mutable` because
`c_str()` is `const` but may have to fill `_str` and re-point `_str_view` at it.

## Construction

The constructors decide whether the wrapper owns a copy or just aliases the
caller's data:

- **`const std::string&`** — `_str_view` aliases the caller's buffer. No copy.
  Because a whole `std::string` is null-terminated, `c_str()` later needs no
  copy either.
- **`std::string&&`** — the string moves into `_str` and `_str_view` points at
  that owned copy. The wrapper now owns the data, so it outlives the caller's
  temporary.
- **`const std::string_view&` / `std::string_view&&`** — `_str_view` is the
  view as given. This also catches string literals and `const char*` (they
  convert to `std::string_view`). The wrapper owns nothing; whether `c_str()`
  copies depends on whether that view happens to be terminated.

## The c_str() contract

`c_str()` must return a null-terminated `const char*`. The trick is doing that
without always copying:

```cpp
auto c_str() const {
    auto str_data = _str.data();
    auto str_view_data = _str_view.data();
    if (str_data != str_view_data && *(str_view_data + _str_view.size()) != '\0') {
        _str.assign(_str_view.data(), _str_view.size());
        _str_view = _str;
        str_view_data = str_data;
    }
    return str_view_data;
}
```

It looks at the byte just past the end of the view. If the view already points
into `_str` (we own it), or the next byte is already `'\0'` (the view spans a
terminated buffer, e.g. a whole `std::string` or a literal), there's nothing to
do and it returns the view's data directly. Only when neither holds, a
non-terminated slice, does it copy the bytes into `_str` and re-point `_str_view`
at the owned copy. So a whole-string or literal wrapper never copies; a substring
copies exactly once, on first `c_str()`.

Reading the byte past the view's end is deliberate and relies on the caller
passing views into buffers that have a readable byte there (which `std::string`
and string literals always do). It is the original Xapiand behavior, preserved.

## Why a wrapper and not just string_view

`std::string_view` is the right parameter type for "any string", but it isn't
null-terminated, so you can't hand its `data()` to a C API expecting a
`const char*`. `std::string` is null-terminated but forces a copy at every call
site that starts from a view or a literal. `stringified` is the middle ground: it
takes the view cheaply and pays for a copy only at the moment, and only in the
case, that a terminated `const char*` is actually demanded.
