/*
 * Copyright © 2026 Germán Méndez Bravo (Kronuz)
 * Copyright (c) 2015-2018 Dubalu LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef STRINGIFIED_HH
#define STRINGIFIED_HH

#if __has_include(<format>)
#include <format>                // for std::formatter
#endif
#include <string>                // for std::string
#include <string_view>           // for std::string_view
#include <ostream>               // for std::ostream
#include <utility>               // for std::move

/*
 * The stringified class is a wrapper for std::string_view which makes
 * it so it ensures c_str() always returns a null-terminated c string.
 * This class only copies the data to ensure this when is needed.
 */
class stringified {
    mutable std::string _str;
    mutable std::string_view _str_view;
    // True when _str_view is known to be followed by a '\0': a whole
    // std::string the caller owns, or our own _str. A bare string_view says
    // nothing about the byte after its end, so it is never assumed terminated.
    mutable bool _terminated = false;

    // The view points into our own _str.
    bool owns() const noexcept {
        return _str_view.data() == _str.data();
    }

    // Point the view at our own storage; call after _str holds the data.
    void bind_own() noexcept {
        _str_view = std::string_view(_str);
        _terminated = true;
    }

public:
    // Copies and moves always leave the view pointing at the new object's own
    // storage when the source's view pointed at the source's storage (the
    // buffer of a short string lives inside the object, so it cannot be
    // carried over by copying the view).
    stringified(const stringified& v)
        : _str_view(v._str_view),
          _terminated(v._terminated)
    {
        if (v.owns()) {
            _str = v._str;
            bind_own();
        }
    }

    stringified(stringified&& v) noexcept
        : _str_view(v._str_view),
          _terminated(v._terminated)
    {
        if (v.owns()) {
            _str = std::move(v._str);
            bind_own();
            v._str.clear();
            v.bind_own();
        }
    }

    stringified& operator=(const stringified& o) {
        if (this != &o) {
            if (o.owns()) {
                _str = o._str;
                bind_own();
            } else {
                _str_view = o._str_view;
                _terminated = o._terminated;
            }
        }
        return *this;
    }

    stringified& operator=(stringified&& o) noexcept {
        if (this != &o) {
            if (o.owns()) {
                _str = std::move(o._str);
                bind_own();
                o._str.clear();
                o.bind_own();
            } else {
                _str_view = o._str_view;
                _terminated = o._terminated;
            }
        }
        return *this;
    }

    explicit stringified(const std::string& str)
        : _str_view(str),
          _terminated(true)
    { }

    explicit stringified(std::string&& str)
        : _str(std::move(str))
    {
        bind_own();
    }

    stringified(const std::string_view& str_view)
        : _str_view(str_view)
    { }

    stringified(std::string_view&& str_view)
        : _str_view(str_view)
    { }

    auto empty() const noexcept {
        return _str_view.empty();
    }

    auto length() const noexcept {
        return _str_view.size();
    }

    // Null-terminated and never reads past the view: a whole std::string and
    // our own storage are returned as is, anything else is copied once.
    auto c_str() const {
        if (!_terminated) {
            _str.assign(_str_view.data(), _str_view.size());
            _str_view = std::string_view(_str);
            _terminated = true;
        }
        return _str_view.data();
    }

    auto size() const noexcept {
        return _str_view.size();
    }

    auto data() const {
        return _str_view.data();
    }

    auto operator[](std::size_t pos) const noexcept {
        return _str_view[pos];
    }

    auto at(std::size_t pos) const {
        return _str_view.at(pos);
    }

    auto front() const noexcept {
        return _str_view.front();
    }

    auto back() const noexcept {
        return _str_view.back();
    }

    auto begin() const noexcept {
        return _str_view.begin();
    }

    auto end() const noexcept {
        return _str_view.end();
    }

    auto cbegin() const noexcept {
        return _str_view.cbegin();
    }

    auto cend() const noexcept {
        return _str_view.cend();
    }

    operator std::string_view() const noexcept {
        return _str_view;
    }

	friend std::ostream& operator<<(std::ostream& os, const stringified& obj) {
		return os << obj._str_view;
	}
};


#if defined(__cpp_lib_format)
// std::format support: a stringified is a string_view wrapper, so format it as one.
template <>
struct std::formatter<stringified> : std::formatter<std::string_view> {
	template <typename Ctx>
	auto format(const stringified& val, Ctx& ctx) const {
		return std::formatter<std::string_view>::format(std::string_view(val), ctx);
	}
};

#endif // __cpp_lib_format

#endif // STRINGIFIED_HH
