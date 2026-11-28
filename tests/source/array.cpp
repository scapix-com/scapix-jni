#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
#include <scapix/jni/array.h>
#include <scapix/jni/string.h>
#include <scapix/jni/convert.h>
#include <scapix/jni/vm_exception.h>
#include <scapix/jni/cast.h>

namespace jni = scapix::jni;

template <typename Elements>
concept can_abort = requires (Elements& e) { e.abort(); };

TEST_CASE("primitive array", "[array]")
{
	auto a = jni::array<jint>::new_array(5);

	REQUIRE(a);
	CHECK(a->size() == 5);

	SECTION("elements are read-only, writable_elements are writable")
	{
		static_assert(std::is_same_v<decltype(a->elements()), jni::array_elements<const jint, jni::lock::noncritical>>);
		static_assert(std::is_same_v<decltype(a->writable_elements()), jni::array_elements<jint, jni::lock::noncritical>>);
		static_assert(std::is_same_v<decltype(a->elements<jni::lock::critical>()), jni::array_elements<const jint, jni::lock::critical>>);
	}

	SECTION("new array is zero initialized")
	{
		auto e = a->elements();
		CHECK(std::all_of(e.begin(), e.end(), [](jint i) { return i == 0; }));
	}

	SECTION("writable elements are written back on destruction")
	{
		{
			auto e = a->writable_elements();
			std::iota(e.begin(), e.end(), 10);
		}

		jint buf[5];
		a->get_region(0, buf);
		CHECK(buf[0] == 10);
		CHECK(buf[4] == 14);
	}

	SECTION("release() writes back before destruction")
	{
		auto e = a->writable_elements();
		e[0] = 42;
		e.release();
		CHECK(e.empty());

		jint value;
		a->get_region(0, std::span(&value, 1));
		CHECK(value == 42);
	}

	SECTION("abort() releases without writing back")
	{
		static_assert(can_abort<jni::array_elements<jint, jni::lock::noncritical>>);
		static_assert(!can_abort<jni::array_elements<const jint, jni::lock::noncritical>>);

		auto e = a->writable_elements();

		// change made to the array while the elements are held: not overwritten by unchanged elements

		const jint in[] = { 42 };
		a->set_region(0, in);

		e.abort();
		CHECK(e.empty());

		jint value;
		a->get_region(0, std::span(&value, 1));
		CHECK(value == 42);
	}

	SECTION("const ref does not make elements read-only (shallow const)")
	{
		const jni::ref<jni::array<jint>>& c = a;
		c->writable_elements()[2] = 3;

		jint value;
		c->get_region(2, std::span(&value, 1));
		CHECK(value == 3);
	}

	SECTION("critical elements")
	{
		{
			auto e = a->writable_elements<jni::lock::critical>();
			e[1] = 7;
		}

		jint value;
		a->get_region(1, std::span(&value, 1));
		CHECK(value == 7);
	}

	SECTION("at() checks range")
	{
		auto e = a->elements();
		CHECK_THROWS_AS(e.at(5), std::out_of_range);
	}

	SECTION("set_region / get_region")
	{
		const jint in[] = { 1, 2, 3 };
		a->set_region(2, in);

		jint out[5];
		a->get_region(0, out);
		CHECK(std::vector<jint>(out, out + 5) == std::vector<jint>{ 0, 0, 1, 2, 3 });

		CHECK_THROWS_AS(a->set_region(4, in), jni::vm_exception);
	}
}

TEST_CASE("primitive array element types", "[array]")
{
	CHECK(jni::array<jboolean>::new_array(1)->size() == 1);
	CHECK(jni::array<jbyte>::new_array(2)->size() == 2);
	CHECK(jni::array<jchar>::new_array(3)->size() == 3);
	CHECK(jni::array<jshort>::new_array(4)->size() == 4);
	CHECK(jni::array<jlong>::new_array(5)->size() == 5);
	CHECK(jni::array<jfloat>::new_array(6)->size() == 6);
	CHECK(jni::array<jdouble>::new_array(7)->size() == 7);
}

TEST_CASE("object array", "[array]")
{
	auto a = jni::array<jni::string>::new_array(3);

	REQUIRE(a);
	CHECK(a->size() == 3);
	CHECK_FALSE(a->empty());
	CHECK_FALSE(a[0].get());

	a[0] = jni::string::new_string_utf("zero");
	a[1] = jni::string::new_string_utf("one");
	a[2] = jni::string::new_string_utf("two");

	CHECK(a[1]->length() == 3);

	SECTION("iteration")
	{
		std::vector<std::string> v;

		for (auto&& i : *a)
			v.push_back(i.get());

		CHECK(v == std::vector<std::string>{ "zero", "one", "two" });
	}

	SECTION("reverse iteration")
	{
		std::vector<std::string> v;

		for (auto i = a->rbegin(); i != a->rend(); ++i)
			v.push_back((*i).get());

		CHECK(v == std::vector<std::string>{ "two", "one", "zero" });
	}

	SECTION("iterator arithmetic")
	{
		auto b = a->begin();
		auto e = a->end();

		CHECK(e - b == 3);
		CHECK(b < e);
		CHECK(std::string(b[2].get()) == "two");
	}

	SECTION("at() checks range")
	{
		CHECK_THROWS_AS(a->at(3), std::out_of_range);
		CHECK_THROWS_AS(a->at(-1), std::out_of_range);
	}

	SECTION("const ref writes elements (shallow const)")
	{
		const jni::ref<jni::array<jni::string>>& c = a;
		c[0] = jni::string::new_string_utf("x");
		(*c)[1] = jni::string::new_string_utf("y");
		CHECK(std::string(a[0].get()) == "x");
		CHECK(std::string(a[1].get()) == "y");
	}

	SECTION("init value")
	{
		auto b = jni::array<jni::string>::new_array(2, jni::string::new_string_utf("x"));
		CHECK(std::string(b[0].get()) == "x");
		CHECK(b[0].get() == b[1].get());
	}

	SECTION("storing wrong type throws ArrayStoreException")
	{
		auto objects = jni::static_pointer_cast<jni::array<jni::object<>>>(jni::ref<jni::array<jni::string>>(a));
		CHECK_THROWS_AS((objects[0] = jni::new_object<jni::object<>, void()>()), jni::vm_exception);
	}
}

TEST_CASE("array conversion", "[array][convert]")
{
	SECTION("std::vector<int>")
	{
		const std::vector<int> v{ 1, 2, 3, 4 };
		jni::ref<jni::array<jint>> j = v;
		CHECK(j->size() == 4);
		std::vector<int> r = j;
		CHECK(r == v);
	}

	SECTION("std::vector<double>")
	{
		const std::vector<double> v{ 1.5, -2.25 };
		jni::ref<jni::array<jdouble>> j = v;
		std::vector<double> r = j;
		CHECK(r == v);
	}

	SECTION("std::vector<bool>")
	{
		const std::vector<bool> v{ true, false, true };
		jni::ref<jni::array<jboolean>> j = v;
		std::vector<bool> r = j;
		CHECK(r == v);
	}

	SECTION("std::vector<std::string>")
	{
		const std::vector<std::string> v{ "a", "bb", "ccc" };
		jni::ref<jni::array<jni::string>> j = v;
		std::vector<std::string> r = j;
		CHECK(r == v);
	}

	SECTION("nested std::vector")
	{
		const std::vector<std::vector<std::string>> v{ { "a" }, {}, { "b", "c" } };
		jni::ref<jni::array<jni::array<jni::string>>> j = v;
		std::vector<std::vector<std::string>> r = j;
		CHECK(r == v);
	}

	SECTION("empty")
	{
		jni::ref<jni::array<jint>> j = std::vector<int>();
		REQUIRE(j);
		CHECK(j->size() == 0);
	}
}
