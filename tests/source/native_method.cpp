#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <stdexcept>
#include <string>
#include <scapix/jni/native_method.h>
#include <scapix/jni/vm_exception.h>

#include "test_class.h"

using namespace test;
using Catch::Matchers::ContainsSubstring;

namespace {

jint native_add(jni::ref<jni::class_>, jint a, jint b)
{
	return a + b;
}

std::string native_greet(jni::ref<test_class> self, std::string name)
{
	return "hello " + name + " " + std::to_string(self->get_field<"intField", jint>());
}

void native_throw()
{
	throw std::runtime_error("thrown from native method");
}

using natives = jni::native_methods
<
	test_class::class_name,
	jni::native_method("nativeAdd", &native_add),
	jni::native_method<jni::ref<jni::string>(jni::ref<jni::string>)>("nativeGreet", &native_greet),
	jni::native_method<void()>("nativeThrow", &native_throw)
>;

void register_natives()
{
	static const bool registered = (natives::register_(), true);
	(void)registered;
}

} // namespace

TEST_CASE("native method: JNI types", "[native]")
{
	register_natives();
	CHECK(test_class::call_static_method<"callNativeAdd", jint(jint, jint)>(20, 22) == 42);
	CHECK(test_class::call_static_method<"nativeAdd", jint(jint, jint)>(1, 2) == 3);
}

TEST_CASE("native method: C++ types", "[native]")
{
	register_natives();
	auto obj = jni::new_object<test_class, void(jint)>(7);
	std::string s = obj->call_method<"callNativeGreet", jni::ref<jni::string>(jni::ref<jni::string>)>("world");
	CHECK(s == "hello world 7");
}

TEST_CASE("native method: C++ exception is visible to Java as com.scapix.NativeException", "[native][exception]")
{
	register_natives();
	std::string s = test_class::call_static_method<"catchNativeThrow", jni::ref<jni::string>()>();
	CHECK(s == "com.scapix.NativeException: thrown from native method");
}

TEST_CASE("native method: C++ exception tunnels through Java back to C++", "[native][exception]")
{
	register_natives();
	CHECK_THROWS_AS((test_class::call_static_method<"callNativeThrow", void()>()), std::runtime_error);
	CHECK_THROWS_WITH((test_class::call_static_method<"callNativeThrow", void()>()), "thrown from native method");
	CHECK_FALSE(jni::detail::env()->ExceptionCheck());
}
