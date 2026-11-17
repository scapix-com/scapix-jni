#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>
#include <scapix/jni/vm_exception.h>
#include <scapix/jni/error.h>

#include "test_class.h"

using namespace test;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("Java exception is thrown as vm_exception", "[exception]")
{
	try
	{
		test_class::call_static_method<"throwIllegalState", void(jni::ref<jni::string>)>("bad state");
		FAIL("exception expected");
	}
	catch (const jni::vm_exception& e)
	{
		CHECK_THAT(e.what(), ContainsSubstring("java.lang.IllegalStateException"));
		CHECK_THAT(e.what(), ContainsSubstring("bad state"));

		CHECK(e.instance_of<jni::object<"java/lang/IllegalStateException", jni::throwable>>());
		CHECK(e.instance_of<jni::object<"java/lang/RuntimeException", jni::throwable>>());
		CHECK_FALSE(e.instance_of<jni::object<"java/io/IOException", jni::throwable>>());
		CHECK(e.get());
	}

	// exception was cleared
	CHECK_FALSE(jni::detail::env()->ExceptionCheck());
}

TEST_CASE("exception from constructor", "[exception]")
{
	using integer = jni::object<"java/lang/Integer">;
	CHECK_THROWS_AS((jni::new_object<integer, void(jni::ref<jni::string>)>("not a number")), jni::vm_exception);
}

TEST_CASE("throwable::throw_", "[exception]")
{
	using runtime_exception = jni::object<"java/lang/RuntimeException", jni::throwable>;

	auto ex = jni::new_object<runtime_exception, void(jni::ref<jni::string>)>("thrown from C++");
	ex->throw_();

	REQUIRE(jni::detail::env()->ExceptionCheck());
	CHECK_THROWS_WITH(jni::detail::check_exception(), ContainsSubstring("thrown from C++"));
	CHECK_FALSE(jni::detail::env()->ExceptionCheck());
}

TEST_CASE("class_::throw_new", "[exception]")
{
	jni::object<"java/lang/UnsupportedOperationException">::class_object()->throw_new("unsupported");
	CHECK_THROWS_WITH(jni::detail::check_exception(), ContainsSubstring("java.lang.UnsupportedOperationException: unsupported"));
}
