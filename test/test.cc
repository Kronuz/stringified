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
#include <string>
#include <string_view>
#include <sstream>

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


int main() {
	test_view_accessors();
	test_construct_from_inputs();
	test_c_str_null_terminated();
	test_iterate_and_stream();
	std::printf("all stringified tests passed\n");
	return 0;
}
