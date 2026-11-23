#include <catch2/catch_test_macros.hpp>

#include <ranges>
#include <string>
#include <type_traits>
#include <vector>
#include <scapix/jni/range.h>
#include <scapix/jni/string.h>
#include <scapix/jni/convert.h>
#include <scapix/jni/vm_exception.h>

namespace jni = scapix::jni;

namespace {

// Base class declared: required by jni::range().
using array_list = jni::object<"java/util/ArrayList", jni::java_lang_iterable>;

jni::local_ref<array_list> make_list(std::initializer_list<const char*> elements)
{
	auto list = jni::new_object<array_list, void()>();

	for (auto e : elements)
		list->call_method<"add", jboolean(jni::ref<>)>(e ? jni::ref<>(jni::string::new_object(e)) : jni::ref<>());

	return list;
}

template <typename Range>
std::vector<std::string> to_vector(Range&& r)
{
	std::vector<std::string> v;

	for (jni::ref<jni::string> s : r)
		v.push_back(s ? std::string(s) : std::string("<null>"));

	return v;
}

template <typename R>
concept can_range = requires (R r) { jni::range(r); };

} // namespace

static_assert(std::ranges::input_range<jni::iterable_range<jni::string>>);
static_assert(std::ranges::input_range<jni::iterator_range<jni::string>>);
static_assert(std::is_same_v<std::ranges::range_reference_t<jni::iterable_range<jni::string>>, jni::ref<jni::string>>);

// Java type must be known statically.

static_assert(can_range<jni::ref<array_list>>);
static_assert(can_range<jni::local_ref<array_list>>);
static_assert(can_range<jni::ref<jni::java_util_iterator>>);
static_assert(!can_range<jni::ref<jni::object<"java/util/ArrayList">>>);
static_assert(!can_range<jni::ref<>>);
static_assert(!can_range<jni::ref<jni::string>>);

static_assert(std::is_same_v<decltype(jni::range(jni::ref<array_list>())), jni::iterable_range<jni::object<>>>);
static_assert(std::is_same_v<decltype(jni::range<jni::string>(jni::ref<jni::java_util_iterator>())), jni::iterator_range<jni::string>>);

TEST_CASE("range: Iterable", "[range]")
{
	auto list = make_list({ "a", "b", nullptr, "d" });
	CHECK(to_vector(jni::range<jni::string>(list)) == std::vector<std::string>{ "a", "b", "<null>", "d" });
}

TEST_CASE("range: raw type with static_pointer_cast", "[range]")
{
	jni::ref<jni::object<"java/util/ArrayList">> raw = make_list({ "x", "y" });
	CHECK(to_vector(jni::range<jni::string>(jni::static_pointer_cast<jni::java_lang_iterable>(raw))) == std::vector<std::string>{ "x", "y" });
}

TEST_CASE("range: Iterable can be iterated more than once", "[range]")
{
	auto list = make_list({ "a", "b" });
	const auto r = jni::range<jni::string>(list);

	CHECK(std::ranges::distance(r) == 2);
	CHECK(std::ranges::distance(r) == 2);
}

TEST_CASE("range: Iterator", "[range]")
{
	auto list = make_list({ "1", "2", "3" });
	auto it = list->call_method<"iterator", jni::ref<jni::java_util_iterator>()>();

	it->call_method<"next", jni::ref<>()>(); // skip first

	CHECK(to_vector(jni::range<jni::string>(it)) == std::vector<std::string>{ "2", "3" });
}

TEST_CASE("range: elements convert to C++ types", "[range]")
{
	std::vector<std::string> v;

	for (std::string s : jni::range<jni::string>(make_list({ "one", "two" }))) // temporary is taken over by range
		v.push_back(s);

	CHECK(v == std::vector<std::string>{ "one", "two" });
}

TEST_CASE("range: element type deduced from parameterized", "[range]")
{
	using string_list = jni::parameterized<array_list, jni::string>;

	auto list = jni::static_pointer_cast<string_list>(make_list({ "g" }));
	auto r = jni::range(list);

	static_assert(std::is_same_v<decltype(r), jni::iterable_range<jni::string>>);

	CHECK(to_vector(r) == std::vector<std::string>{ "g" });
}

TEST_CASE("range: empty", "[range]")
{
	CHECK(to_vector(jni::range<jni::string>(make_list({}))).empty());
	CHECK(to_vector(jni::range<jni::string>(jni::ref<array_list>())).empty());
	CHECK(to_vector(jni::range<jni::string>(jni::ref<jni::java_util_iterator>())).empty());
}

TEST_CASE("range: long collection", "[range]")
{
	auto list = jni::new_object<array_list, void()>();

	for (jint i = 0; i < 1000; ++i)
		list->call_method<"add", jboolean(jni::ref<>)>(jni::object<"java/lang/Integer">::call_static_method<"valueOf", jni::ref<jni::object<"java/lang/Integer">>(jint)>(i));

	jlong sum = 0;

	for (auto i : jni::range<jni::object<"java/lang/Integer">>(list))
		sum += i->call_method<"intValue", jint()>();

	CHECK(sum == 999 * 1000 / 2);
}

TEST_CASE("range: Java exception during iteration", "[range]")
{
	auto list = make_list({ "a", "b" });
	auto r = jni::range<jni::string>(list);
	auto i = r.begin();

	list->call_method<"add", jboolean(jni::ref<>)>(jni::string::new_object("c"));

	CHECK_THROWS_AS(++i, jni::vm_exception); // ConcurrentModificationException
}
