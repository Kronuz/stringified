// Smoke test for the standalone stringified library.
//
// Exercises stringified.hh: the wrapper that coerces a std::string, a
// std::string_view, or a string literal into one std::string_view-friendly
// surface with optional backing storage. The interesting bit is c_str(): the
// view it hands back is guaranteed null-terminated, copying into the backing
// std::string only when the wrapped view isn't already terminated. The test
// checks the view accessors round-trip, that c_str() is always null-terminated,
// and that it copies only when it has to.
//
// Build via CMake: cmake -B build && cmake --build build && ctest --test-dir build
#include <cassert>
#include <cstdio>
#include <cstring>
#if __has_include(<format>)
#include <format>
#endif
#include <string>
#include <string_view>
#include <sstream>

#include <vector>
#include "stringified.hh"


// ---------------------------------------------------------------------------
// View accessors: size/length/empty/data/indexing all reflect the wrapped
// value, whichever input type it came from.
// ---------------------------------------------------------------------------

static void test_view_accessors() {
	const std::string owned = "hello";
	stringified s(owned);

	assert(!s.empty());
	assert(s.size() == 5);
	assert(s.length() == 5);
	assert(s[0] == 'h');
	assert(s.at(4) == 'o');
	assert(s.front() == 'h');
	assert(s.back() == 'o');
	assert(std::string_view(s) == "hello");
	assert(std::memcmp(s.data(), "hello", 5) == 0);

	stringified empty(std::string_view{});
	assert(empty.empty());
	assert(empty.size() == 0);

	std::printf("stringified view-accessors OK: size/empty/index/data reflect the wrapped value\n");
}


// ---------------------------------------------------------------------------
// Construction from each accepted input type lands on the same view contents.
// ---------------------------------------------------------------------------

static void test_construct_from_inputs() {
	// From an lvalue std::string: the view aliases the caller's buffer.
	const std::string owned = "from-string";
	stringified a(owned);
	assert(std::string_view(a) == "from-string");

	// From an rvalue std::string: the wrapper takes ownership in its backing
	// store and the view points at that copy.
	stringified b(std::string("from-rvalue"));
	assert(std::string_view(b) == "from-rvalue");

	// From a std::string_view.
	std::string_view sv = "from-view";
	stringified c(sv);
	assert(std::string_view(c) == "from-view");

	// From a string literal (binds via the string_view ctor).
	stringified d(std::string_view{"from-literal"});
	assert(std::string_view(d) == "from-literal");

	std::printf("stringified construct OK: string / string&& / string_view / literal all match\n");
}


// ---------------------------------------------------------------------------
// c_str(): the returned pointer is always null-terminated. When the wrapped
// view already sits inside a null-terminated buffer (a whole std::string), no
// copy is needed and c_str() points back into it; when it's a substring that
// isn't terminated, c_str() copies into the backing store first.
// ---------------------------------------------------------------------------

static void test_c_str_null_terminated() {
	// Whole std::string: data already null-terminated, c_str() can alias it.
	const std::string owned = "terminated";
	stringified s(owned);
	const char* p = s.c_str();
	assert(std::strcmp(p, "terminated") == 0);
	assert(p[s.size()] == '\0');

	// A non-terminated slice: take a view over the first 3 chars of a longer
	// buffer. The char past the slice is 'd', not '\0', so c_str() must copy
	// into the backing store to give a terminated string.
	const std::string longer = "abcdef";
	std::string_view slice(longer.data(), 3);   // "abc", not null-terminated
	assert(slice.data()[slice.size()] != '\0'); // sanity: the char after is 'd'
	stringified sliced(slice);
	const char* q = sliced.c_str();
	assert(std::strcmp(q, "abc") == 0);          // exactly 3 chars, then NUL
	assert(q[3] == '\0');
	assert(std::string_view(sliced) == "abc");   // view still reads the slice

	std::printf("stringified c_str OK: always null-terminated, copies only when the slice isn't\n");
}


// ---------------------------------------------------------------------------
// Iteration and ostream insertion: begin/end span the view, operator<< writes
// the wrapped contents.
// ---------------------------------------------------------------------------

static void test_iterate_and_stream() {
	const std::string owned = "xyz";
	stringified s(owned);

	std::string rebuilt(s.begin(), s.end());
	assert(rebuilt == "xyz");
	std::string rebuilt_c(s.cbegin(), s.cend());
	assert(rebuilt_c == "xyz");

	std::ostringstream os;
	os << s;
	assert(os.str() == "xyz");

	std::printf("stringified iterate/stream OK: begin/end span the view, operator<< writes it\n");
}


// ---------------------------------------------------------------------------
// std::format support: the std::formatter<stringified> specialization formats a
// stringified exactly as its underlying string_view, including format specs.
// ---------------------------------------------------------------------------

// libc++ 17 and 18 ship a usable <format> but do not define __cpp_lib_format.
#if defined(__cpp_lib_format) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 170000)
static void test_std_format() {
	const std::string owned = "world";
	stringified s(owned);

	assert(std::format("{}", s) == "world");
	assert(std::format("hello {}", s) == "hello world");
	assert(std::format("[{:>7}]", s) == "[  world]");   // a spec forwarded to string_view

	std::printf("stringified std::format OK: formats as its string_view, honoring specs\n");
}

#endif

// ---------------------------------------------------------------------------
// stringified(std::string&&) takes ownership: a heap-allocated string is moved
// (same buffer), not copied.
// ---------------------------------------------------------------------------

static void test_rvalue_string_is_moved() {
	std::string big(200, 'x');
	const char* original = big.data();
	stringified s(std::move(big));
	assert(s.data() == original);
	assert(s.size() == 200);
	assert(s.c_str()[200] == '\0');

	std::string small("abc");
	stringified t(std::move(small));
	assert(std::string_view(t) == "abc");

	std::printf("stringified rvalue string OK: heap buffer moved, not copied\n");
}


// ---------------------------------------------------------------------------
// Copy and move: the view must point at the object's own storage, never into a
// source that is gone or moved from. Sizes 0..80 cover the short-string buffer
// (inside the object) and the heap. A view copied verbatim reads freed or stale
// memory here under ASan.
// ---------------------------------------------------------------------------

static std::string make(std::size_t n) {
	std::string r;
	for (std::size_t i = 0; i < n; ++i) r.push_back(char('a' + i % 26));
	return r;
}

static void check(const stringified& s, const std::string& want) {
	assert(std::string_view(s) == want);
	assert(s.size() == want.size());
	const char* p = s.c_str();
	assert(std::string_view(p, want.size()) == want);
	assert(p[want.size()] == '\0');
	assert(s.data() == p);
}

static void test_copy_move_own_storage() {
	for (std::size_t n = 0; n <= 80; ++n) {
		const std::string want = make(n);

		// copy construct from an owning source, then destroy the source
		{
			auto* src = new stringified(std::string(want));
			stringified c(*src);
			delete src;
			check(c, want);
		}
		// copy construct: the copy has its own buffer (distinct from source)
		{
			stringified src{std::string(want)};
			stringified c(src);
			assert(c.data() != src.data());
			check(c, want);
			check(src, want);
		}
		// move construct, source destroyed / reused
		{
			auto* src = new stringified(std::string(want));
			stringified m(std::move(*src));
			delete src;
			check(m, want);
		}
		// copy / move assignment from owning source into owning and viewing targets
		for (int target = 0; target < 2; ++target) {
			const std::string other = make(n + 7);
			stringified src{std::string(want)};
			stringified t = target ? stringified(std::string(other)) : stringified(std::string_view(other));
			t = src;
			assert(t.data() != src.data() || n == 0);
			check(t, want);
			check(src, want);
			stringified t2 = target ? stringified(std::string(other)) : stringified(std::string_view(other));
			{
				stringified src2{std::string(want)};
				t2 = std::move(src2);
			}
			check(t2, want);
		}
		// copy of a copy through a vector (reallocation moves every element)
		{
			std::vector<stringified> v;
			for (int i = 0; i < 9; ++i) v.emplace_back(std::string(want + char('0' + i)));
			for (int i = 0; i < 9; ++i) check(v[i], want + char('0' + i));
		}
		// self-assignment
		{
			stringified s{std::string(want)};
			stringified& r = s;
			s = r;
			check(s, want);
		}
		// copies of views alias the original buffer, unchanged
		{
			const std::string ext = want;
			stringified a{std::string_view(ext)};
			stringified b(a);
			assert(b.data() == ext.data());
			check(stringified(std::string_view(ext)), want);
		}
	}
	std::printf("stringified copy/move OK: the view always points at the object's own storage\n");
}


// ---------------------------------------------------------------------------
// c_str() never reads past a view: a view over a heap buffer with no byte after
// it (exact allocation, no terminator) is copied; the byte after it is never read.
// ---------------------------------------------------------------------------

static void test_c_str_cstring_aliases() {
	const char* lit = "hello";
	stringified a(lit);
	assert(a.c_str() == lit);          // C string: aliased, no copy
	assert(a.length() == 5);
	stringified b("literal");
	assert(std::strcmp(b.c_str(), "literal") == 0);
	stringified c(std::string_view(lit, 3));
	assert(c.c_str() != lit && std::strcmp(c.c_str(), "hel") == 0);  // bare view: copied
	stringified d(a);
	assert(d.c_str() == lit);          // copies keep the terminated state
	std::printf("stringified c_str OK: C strings alias\n");
}

static void test_c_str_no_overread() {
	for (std::size_t n = 0; n <= 70; ++n) {
		char* buf = new char[n ? n : 1];
		for (std::size_t i = 0; i < n; ++i) buf[i] = char('A' + i % 26);
		stringified s{std::string_view(buf, n)};
		const char* p = s.c_str();
		assert(p != buf);
		assert(std::string_view(p, n) == std::string_view(buf, n));
		assert(p[n] == '\0');
		delete[] buf;                       // c_str() result is independent of buf
		assert(std::string_view(s) == make(0).assign(p, n));
		const std::string whole = make(n);  // a whole std::string is aliased, no copy
		stringified w(whole);
		assert(w.c_str() == whole.c_str());
	}
	std::printf("stringified c_str OK: no read past a string_view\n");
}


int main() {
	test_copy_move_own_storage();
	test_c_str_no_overread();
	test_c_str_cstring_aliases();
	test_rvalue_string_is_moved();
	test_view_accessors();
	test_construct_from_inputs();
	test_c_str_null_terminated();
	test_iterate_and_stream();
#if defined(__cpp_lib_format) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 170000)
	test_std_format();
#endif
	std::printf("all stringified tests passed\n");
	return 0;
}
