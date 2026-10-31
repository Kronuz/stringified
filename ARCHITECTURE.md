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
- **`const char*`** (including string literals) — the view spans the C string
  and `_terminated` is set, so `c_str()` aliases it.
- **`const std::string_view&` / `std::string_view&&`** — `_str_view` is the
  view as given. The wrapper owns nothing; the byte after the view is unknown,
  so the first `c_str()` copies.

## The c_str() contract

`c_str()` must return a null-terminated `const char*`. The trick is doing that
without always copying:

```cpp
auto c_str() const {
    if (!_terminated) {
        _str.assign(_str_view.data(), _str_view.size());
        _str_view = std::string_view(_str);
        _terminated = true;
    }
    return _str_view.data();
}
```

`_terminated` is set when the view is a whole `std::string` the caller owns, a
`const char*` or literal, or our own `_str`; then `c_str()` returns the view's data directly. For anything
else (a `string_view`, including a slice) the byte after the view
is unknown and is never read: the bytes are copied into `_str` once, on first
`c_str()`, and the view is re-pointed at that copy.

Copy and move construction/assignment rebind the view to the destination's own
`_str` whenever the source's view pointed at the source's `_str`; a short
string's buffer is inside the object, so copying the view alone would dangle.

## Why a wrapper and not just string_view

`std::string_view` is the right parameter type for "any string", but it isn't
null-terminated, so you can't hand its `data()` to a C API expecting a
`const char*`. `std::string` is null-terminated but forces a copy at every call
site that starts from a view or a literal. `stringified` is the middle ground: it
takes the view cheaply and pays for a copy only at the moment, and only in the
case, that a terminated `const char*` is actually demanded.
