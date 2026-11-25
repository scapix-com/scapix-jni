#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <functional>
#include <scapix/jni/native_method.h>
#include <scapix/jni/bridge/function.h>

namespace jni = scapix::jni;

// tests/java/com/scapix/test/IntFunction.java

namespace function_test {

using int_function = jni::function<"com/scapix/test/IntFunction", jint(jint)>;
using int_function_impl = jni::bridge::function_impl<"com/scapix/test/IntFunction", jint(jint), "call">;
using cpp_function_impl = jni::bridge::cpp::function_impl<jint(jint)>;

using natives = jni::native_methods
<
	int_function_impl::class_name,
	jni::native_method("call", &cpp_function_impl::call)
>;

void register_natives()
{
	static const bool registered = (natives::register_(), true);
	(void)registered;
}

// Distinct C++ signature for the same Java interface.

enum class number : std::int32_t {};

struct add
{
	jint operator()(jint v) { return total += v; }
	jint total = 0;
};

TEST_CASE("function: C++ -> Java -> C++ unwraps std::function", "[function]")
{
	register_natives();

	std::function<jint(jint)> f = add{ 10 };
	auto j = jni::convert_jni<jni::ref<int_function>>(f);

	CHECK(j->is_instance_of<int_function_impl>());
	CHECK(int_function::call_static_method<"apply", jint(jni::ref<int_function>, jint)>(j, 1) == 11);

	auto g = jni::convert_cpp<std::function<jint(jint)>>(j);

	REQUIRE(g.target<add>());
	CHECK(g(1) == 12); // copy of the wrapped function, including its state
}

TEST_CASE("function: Java -> C++ -> Java unwraps Java object", "[function]")
{
	auto j = int_function::call_static_method<"twice", jni::ref<int_function>()>();
	auto f = jni::convert_cpp<std::function<jint(jint)>>(j);

	CHECK(f(21) == 42);

	auto j2 = jni::convert_jni<jni::ref<int_function>>(f);

	CHECK(j2 == j);
	CHECK(j2.get_scope() == jni::scope::local);
}

TEST_CASE("function: different C++ signature is wrapped", "[function]")
{
	register_natives();

	std::function<jint(jint)> f = add{ 10 };
	auto j = jni::convert_jni<jni::ref<int_function>>(f);

	auto g = jni::convert_cpp<std::function<jint(number)>>(j);

	CHECK(!g.target<add>());
	CHECK(g(number{ 1 }) == 11); // calls through Java
	CHECK(g(number{ 1 }) == 12);
}

TEST_CASE("function: null", "[function]")
{
	CHECK(!jni::convert_jni<jni::ref<int_function>>(std::function<jint(jint)>()));
	CHECK(!jni::convert_cpp<std::function<jint(jint)>>(jni::ref<int_function>()));
}

} // namespace function_test
