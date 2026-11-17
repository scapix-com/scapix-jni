#include <catch2/catch_test_macros.hpp>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <scapix/jni/convert.h>

namespace jni = scapix::jni;

template <typename J, typename Cpp>
Cpp round_trip(const Cpp& value)
{
	return jni::convert_cpp<Cpp>(jni::convert_jni<jni::ref<J>>(value));
}

TEST_CASE("convert boxed primitives", "[convert]")
{
	CHECK(round_trip<jni::object<"java/lang/Boolean">>(true) == true);
	CHECK(round_trip<jni::object<"java/lang/Byte">>(std::int8_t(-5)) == -5);
	CHECK(round_trip<jni::object<"java/lang/Short">>(std::int16_t(1000)) == 1000);
	CHECK(round_trip<jni::object<"java/lang/Integer">>(123456) == 123456);
	CHECK(round_trip<jni::object<"java/lang/Long">>(std::int64_t(1) << 40) == std::int64_t(1) << 40);
	CHECK(round_trip<jni::object<"java/lang/Float">>(1.5f) == 1.5f);
	CHECK(round_trip<jni::object<"java/lang/Double">>(2.25) == 2.25);
}

TEST_CASE("convert enum", "[convert]")
{
	enum class color : std::int32_t { red = 1, green = 2 };

	CHECK(round_trip<jni::object<"java/lang/Integer">>(color::green) == color::green);
}

TEST_CASE("convert std::shared_ptr", "[convert]")
{
	auto p = round_trip<jni::string>(std::make_shared<std::string>("shared"));
	REQUIRE(p);
	CHECK(*p == "shared");

	CHECK_FALSE(jni::convert_jni<jni::ref<jni::string>>(std::shared_ptr<std::string>()));
}

TEST_CASE("convert std::map", "[convert]")
{
	const std::map<std::string, int> m{ { "one", 1 }, { "two", 2 }, { "three", 3 } };

	SECTION("java.util.TreeMap")
	{
		using map = jni::generic_type<jni::java_util_treemap, jni::string, jni::object<"java/lang/Integer">>;
		auto j = jni::convert_jni<jni::ref<map>>(m);
		CHECK(j->call_method<"size", jint()>() == 3);
		CHECK((jni::convert_cpp<std::map<std::string, int>>(jni::ref<map>(j)) == m));
	}

	SECTION("java.util.Map")
	{
		using map = jni::generic_type<jni::java_util_map, jni::string, jni::object<"java/lang/Integer">>;
		auto j = jni::convert_jni<jni::ref<map>>(m);
		CHECK((jni::convert_cpp<std::map<std::string, int>>(jni::ref<map>(j)) == m));
	}
}

TEST_CASE("convert std::set", "[convert]")
{
	using set = jni::generic_type<jni::java_util_treeset, jni::string>;
	const std::set<std::string> s{ "b", "a", "c" };
	auto j = jni::convert_jni<jni::ref<set>>(s);
	CHECK(j->call_method<"size", jint()>() == 3);
	CHECK(jni::convert_cpp<std::set<std::string>>(jni::ref<set>(j)) == s);
}

TEST_CASE("convert std::unordered_map", "[convert]")
{
	using map = jni::generic_type<jni::java_util_hashmap, jni::string, jni::string>;
	const std::unordered_map<std::string, std::string> m{ { "k1", "v1" }, { "k2", "v2" } };
	auto j = jni::convert_jni<jni::ref<map>>(m);
	CHECK((jni::convert_cpp<std::unordered_map<std::string, std::string>>(jni::ref<map>(j)) == m));
}

TEST_CASE("convert std::unordered_set", "[convert]")
{
	using set = jni::generic_type<jni::java_util_hashset, jni::object<"java/lang/Integer">>;
	const std::unordered_set<int> s{ 1, 2, 3 };
	auto j = jni::convert_jni<jni::ref<set>>(s);
	CHECK(jni::convert_cpp<std::unordered_set<int>>(jni::ref<set>(j)) == s);
}
