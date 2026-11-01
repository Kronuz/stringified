// Overload resolution and C-string aliasing of the stringified(const char*) constructor.
//
// A constructor that takes a C string must not give a string literal a second
// user-defined conversion: f(stringified) next to f(std::string_view) (or
// g(const std::string&)) has to stay unambiguous for f("abc"). And a type that
// converts to both const char* and std::string_view (static_string does) must
// still construct a stringified. Both are compile-time properties, so this file
// fails to build when they break; the runtime checks pin which overload runs and
// that a C string aliases in c_str().
//
// CHECK aborts instead of asserting, so the test still checks under NDEBUG.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>

#include "stringified.hh"

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
			std::abort(); \
		} \
	} while (0)


// The overload sets of the audit finding: a literal must pick the other overload.
static int f(stringified) { return 1; }
static int f(std::string_view) { return 2; }
static int g(stringified) { return 1; }
static int g(const std::string&) { return 2; }
// With nothing else to pick, the caller wraps explicitly.
static int only(const stringified& s) { return static_cast<int>(s.size()); }

// Converts to both const char* and std::string_view, like static_string.
struct Both {
	const char* text = "both";
	operator const char*() const { return text; }
	operator std::string_view() const { return std::string_view(text, 4); }
};

static_assert(std::is_constructible_v<stringified, const char*>);
static_assert(std::is_constructible_v<stringified, char*>);
static_assert(std::is_constructible_v<stringified, const char (&)[4]>);
static_assert(std::is_constructible_v<stringified, char (&)[8]>);
static_assert(std::is_constructible_v<stringified, std::string_view>);
static_assert(std::is_constructible_v<stringified, const std::string&>);
static_assert(std::is_constructible_v<stringified, std::string>);
static_assert(std::is_constructible_v<stringified, Both>);

static_assert(!std::is_convertible_v<const char*, stringified>);
static_assert(!std::is_convertible_v<char*, stringified>);
static_assert(!std::is_convertible_v<const char (&)[4], stringified>);
static_assert(!std::is_convertible_v<Both, stringified>);
static_assert(std::is_convertible_v<std::string_view, stringified>);
static_assert(!std::is_convertible_v<std::string, stringified>);
static_assert(!std::is_constructible_v<stringified, const int*>);


static void test_literal_picks_the_other_overload() {
	char buf[8] = "abc";
	const char* p = "abc";
	CHECK(f("abc") == 2);
	CHECK(f(p) == 2);
	CHECK(f(buf) == 2);
	CHECK(g("abc") == 2);
	CHECK(g(p) == 2);
	CHECK(g(buf) == 2);
	CHECK(f(stringified("abc")) == 1);
	CHECK(g(stringified(p)) == 1);
	CHECK(only(stringified("abc")) == 3);
	std::printf("stringified overloads OK: literals pick string_view / std::string\n");
}

static void test_c_strings_alias() {
	const char* p = "hello";
	char buf[16] = "from-buffer";
	char* mp = buf;

	stringified a(p);
	CHECK(a.c_str() == p && a.size() == 5);
	stringified b("literal");
	CHECK(b.size() == 7 && std::string_view(b.c_str()) == "literal");
	stringified c(buf);
	CHECK(c.c_str() == buf && c.size() == 11);
	stringified d(mp);
	CHECK(d.c_str() == buf);
	stringified e{p};
	CHECK(e.c_str() == p);
	stringified copy(a);
	CHECK(copy.c_str() == p);
	stringified view(std::string_view(p, 3));
	CHECK(view.c_str() != p && std::string_view(view.c_str()) == "hel");
	std::printf("stringified overloads OK: C strings alias in c_str()\n");
}

static void test_both_conversions_take_the_view_constructor() {
	Both both;
	stringified s(both);
	CHECK(s.size() == 4 && std::string_view(s) == "both");
	const char* p = s.c_str();
	CHECK(std::string_view(p) == "both");
	CHECK(p != both.text);                // the view constructor copied: it is not a C string
	std::printf("stringified overloads OK: a type convertible to both takes the view constructor\n");
}


int main() {
	test_literal_picks_the_other_overload();
	test_c_strings_alias();
	test_both_conversions_take_the_view_constructor();
	std::printf("all stringified overload tests passed\n");
	return 0;
}
