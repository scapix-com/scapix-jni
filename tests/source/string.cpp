#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>
#include <scapix/jni/string.h>
#include <scapix/jni/convert.h>
#include <scapix/jni/vm_exception.h>

namespace jni = scapix::jni;

namespace {

// jchar is not char16_t on all platforms (unsigned short on Windows).

std::u16string to_u16(std::basic_string_view<jchar> view)
{
	return std::u16string(view.begin(), view.end());
}

} // namespace

TEST_CASE("string::new_object from modified UTF-8", "[string]")
{
	auto s = jni::string::new_object("hello");

	REQUIRE(s);
	CHECK(s->length() == 5);
	CHECK(s->length<char>() == 5);
}

TEST_CASE("string::new_object from UTF-16", "[string]")
{
	const jchar buf[] = { 'a', 'b', 0x05D0 };
	auto s = jni::string::new_object(buf, 3);

	CHECK(s->length() == 3);
	CHECK(s->length<char>() == 4); // U+05D0 is 2 bytes in (modified) UTF-8
}

TEST_CASE("string::chars", "[string]")
{
	auto s = jni::string::new_object("abc");

	SECTION("UTF-16")
	{
		auto chars = s->chars();
		REQUIRE(chars.size() == 3);
		CHECK(chars[0] == 'a');
		CHECK(to_u16(chars.view()) == u"abc");
		CHECK_THROWS_AS(chars.at(3), std::out_of_range);

		chars.release();
		CHECK(chars.empty());
	}

	SECTION("modified UTF-8")
	{
		auto chars = s->chars<char>();
		CHECK(chars.view() == "abc");
	}

	SECTION("critical")
	{
		auto chars = s->chars<jchar, jni::lock::critical>();
		CHECK(to_u16(chars.view()) == u"abc");
	}

	SECTION("partial")
	{
		auto chars = s->chars(2);
		CHECK(to_u16(chars.view()) == u"ab");
	}
}

TEST_CASE("string::get_region", "[string]")
{
	auto s = jni::string::new_object("abcdef");

	jchar buf[3];
	s->get_region(1, 3, buf);
	CHECK(to_u16({ buf, 3 }) == u"bcd");

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
