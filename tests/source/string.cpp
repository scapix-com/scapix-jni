#include <catch2/catch_test_macros.hpp>

#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <scapix/jni/string.h>
#include <scapix/jni/convert.h>
#include <scapix/jni/vm_exception.h>

namespace jni = scapix::jni;

namespace {

// jchar is not char16_t on all platforms (unsigned short on Windows, uint16_t on Android),
// and std::basic_string_view<jchar> needs std::char_traits<jchar>, which libc++ 19+ does not provide.

std::u16string to_u16(std::span<const jchar> chars)
{
	return std::u16string(chars.begin(), chars.end());
}

} // namespace

TEST_CASE("string::new_string_utf", "[string]")
{
	auto s = jni::string::new_string_utf("hello");

	REQUIRE(s);
	CHECK(s->length() == 5);
	CHECK(s->length<char>() == 5);
}

TEST_CASE("string::new_string", "[string]")
{
	const jchar buf[] = { 'a', 'b', 0x05D0 };
	auto s = jni::string::new_string(buf, 3);

	CHECK(s->length() == 3);
	CHECK(s->length<char>() == 4); // U+05D0 is 2 bytes in (modified) UTF-8
}

TEST_CASE("string::new_string from span", "[string]")
{
	SECTION("array")
	{
		const jchar buf[] = { 'a', 'b', 0x05D0 };
		auto s = jni::string::new_string(std::span(buf));

		CHECK(to_u16(s->chars()) == std::u16string{ u'a', u'b', char16_t(0x05D0) });
	}

	SECTION("subspan (not null terminated)")
	{
		const std::vector<jchar> buf = { 'x', 'a', 'b', 'c', 'y' };
		auto s = jni::string::new_string(std::span(buf).subspan(1, 3));

		CHECK(s->length() == 3);
		CHECK(to_u16(s->chars()) == u"abc");
	}

	SECTION("embedded null")
	{
		const jchar buf[] = { 'a', 0, 'b' };
		auto s = jni::string::new_string(std::span(buf));

		CHECK(s->length() == 3);
		CHECK(s->length<char>() == 4); // U+0000 is 2 bytes (C0 80) in modified UTF-8
		CHECK(to_u16(s->chars()) == std::u16string(u"a\0b", 3));
	}

	SECTION("empty")
	{
		auto s = jni::string::new_string(std::span<const jchar>());

		REQUIRE(s);
		CHECK(s->length() == 0);
	}

	SECTION("round trip from string_chars")
	{
		auto original = jni::string::new_string_utf("hello");
		auto copy = jni::string::new_string(original->chars());

		CHECK(copy != original);
		CHECK(to_u16(copy->chars()) == u"hello");
	}
}

TEST_CASE("string::chars", "[string]")
{
	auto s = jni::string::new_string_utf("abc");

	SECTION("UTF-16")
	{
		auto chars = s->chars();
		REQUIRE(chars.size() == 3);
		CHECK(chars[0] == 'a');
		CHECK(to_u16(chars) == u"abc");
		CHECK_THROWS_AS(chars.at(3), std::out_of_range);

		chars.release();
		CHECK(chars.empty());
	}

	SECTION("modified UTF-8")
	{
		auto chars = s->chars<char>();
		CHECK(std::string_view(chars.data(), chars.size()) == "abc");
	}

	SECTION("critical")
	{
		auto chars = s->chars<jchar, jni::lock::critical>();
		CHECK(to_u16(chars) == u"abc");
	}

	SECTION("partial")
	{
		auto chars = s->chars(2);
		CHECK(to_u16(chars) == u"ab");
	}
}

TEST_CASE("string::get_region", "[string]")
{
	auto s = jni::string::new_string_utf("abcdef");

	jchar buf[3];
	s->get_region(1, 3, buf);
	CHECK(to_u16(buf) == u"bcd");

	CHECK_THROWS_AS(s->get_region(5, 3, buf), jni::vm_exception);
}

TEST_CASE("string conversion", "[string][convert]")
{
	SECTION("ASCII")
	{
		jni::ref<jni::string> j = std::string("hello world");
		std::string s = j;
		CHECK(s == "hello world");
	}

	SECTION("empty")
	{
		jni::ref<jni::string> j = std::string();
		REQUIRE(j);
		CHECK(j->length() == 0);
		std::string s = j;
		CHECK(s.empty());
	}

	SECTION("non-ASCII round trip")
	{
		const std::string original = "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D \xF0\x9F\x98\x80"; // Hebrew + emoji (4 byte UTF-8)
		jni::ref<jni::string> j = original;
		std::string s = j;
		CHECK(s == original);
	}

	SECTION("null ref converts to empty string")
	{
		jni::ref<jni::string> j;
		std::string s = j;
		CHECK(s.empty());
	}
}
