#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>
#include <scapix/jni/object.h>
#include <scapix/jni/string.h>
#include <scapix/jni/array.h>
#include <scapix/jni/class.h>
#include <scapix/jni/cast.h>
#include <scapix/jni/convert.h>
#include <scapix/jni/vm_exception.h>

#include "test_class.h"

using namespace test;

TEST_CASE("class_object", "[object]")
{
	SECTION("platform class")
	{
		auto cls = jni::string::class_object();
		REQUIRE(cls);
		std::string name = cls->call_method<"getName", jni::ref<jni::string>()>();
		CHECK(name == "java.lang.String");
	}

	SECTION("application class")
	{
		auto cls = test_class::class_object();
		REQUIRE(cls);
		std::string name = cls->call_method<"getName", jni::ref<jni::string>()>();
		CHECK(name == "com.scapix.test.TestClass");
	}

	SECTION("array class")
	{
		auto cls = jni::array<jni::string>::class_object();
		REQUIRE(cls);
		std::string name = cls->call_method<"getName", jni::ref<jni::string>()>();
		CHECK(name == "[Ljava.lang.String;");
	}

	SECTION("cached")
	{
		CHECK(test_class::class_object() == test_class::class_object());
	}

	SECTION("superclass")
	{
		CHECK(test_class::class_object()->get_superclass() == jni::object<>::class_object());
		CHECK(jni::object<>::class_object()->is_assignable_from(test_class::class_object()));
		CHECK_FALSE(test_class::class_object()->is_assignable_from(jni::object<>::class_object()));
	}
}

TEST_CASE("class_::find_class", "[object]")
{
	CHECK(jni::class_::find_class("java/lang/String") == jni::string::class_object());
	CHECK_THROWS_AS(jni::class_::find_class("does/not/Exist"), jni::vm_exception);
}

TEST_CASE("new_object", "[object]")
{
	SECTION("default constructor")
	{
		auto obj = jni::new_object<test_class, void()>();
		REQUIRE(obj);
		CHECK(obj->get_field<"intField", jint>() == 1);
	}

	SECTION("constructor with parameters")
	{
		auto obj = jni::new_object<test_class, void(jint)>(42);
		CHECK(obj->get_field<"intField", jint>() == 42);
	}

	SECTION("class with new_object() static member")
	{
		auto s = jni::new_object<jni::string>("abc");
		CHECK(s->length() == 3);
	}
}

TEST_CASE("call methods", "[object]")
{
	auto obj = jni::new_object<test_class, void()>();

	SECTION("instance method")
	{
		CHECK(obj->call_method<"add", jint(jint, jint)>(2, 3) == 5);
	}

	SECTION("nonvirtual method")
	{
		CHECK(obj->call_nonvirtual_method<"add", jint(jint, jint)>(4, 5) == 9);
	}

	SECTION("static method with converted parameters")
	{
		std::string s = test_class::call_static_method<"concat", jni::ref<jni::string>(jni::ref<jni::string>, jni::ref<jni::string>)>("ab", std::string("cd"));
		CHECK(s == "abcd");
	}

	SECTION("static method returning primitive array")
	{
		std::vector<int> v = test_class::call_static_method<"range", jni::ref<jni::array<jint>>(jint)>(4);
		CHECK(v == std::vector<int>{ 0, 1, 2, 3 });
	}

	SECTION("method of base class")
	{
		std::string s = obj->call_method<"toString", jni::ref<jni::string>()>();
		CHECK(s.starts_with("com.scapix.test.TestClass@"));
	}

	SECTION("explicit method id")
	{
		auto id = test_class::class_object()->get_method_id("add", "(II)I");
		CHECK(obj->call_method<jint(jint, jint)>(id, 1, 1) == 2);
	}

	SECTION("missing method throws NoSuchMethodError")
	{
		REQUIRE_THROWS_AS(test_class::class_object()->get_method_id("noSuchMethod", "()V"), jni::vm_exception);
	}
}

TEST_CASE("fields", "[object]")
{
	auto obj = jni::new_object<test_class, void()>();

	SECTION("primitive field")
	{
		obj->set_field<"intField", jint>(123);
		CHECK(obj->get_field<"intField", jint>() == 123);
	}

	SECTION("object field")
	{
		std::string s = obj->get_field<"stringField", jni::ref<jni::string>>();
		CHECK(s == "field");

		obj->set_field<"stringField", jni::ref<jni::string>>(jni::string::new_object("changed"));
		s = obj->get_field<"stringField", jni::ref<jni::string>>();
		CHECK(s == "changed");

		obj->set_field<"stringField", jni::ref<jni::string>>(nullptr);
		CHECK_FALSE(obj->get_field<"stringField", jni::ref<jni::string>>());
	}

	SECTION("static field")
	{
		CHECK(test_class::get_static_field<"staticIntField", jint>() == 2);
		test_class::set_static_field<"staticIntField", jint>(3);
		CHECK(test_class::get_static_field<"staticIntField", jint>() == 3);
		test_class::set_static_field<"staticIntField", jint>(2);
	}
}

TEST_CASE("reflection", "[object]")
{
	auto method = test_class::to_reflected_method<"add", jint(jint, jint)>();
	std::string name = method->call_method<"getName", jni::ref<jni::string>()>();
	CHECK(name == "add");
	CHECK(jni::class_::from_reflected_method(method) == test_class::class_object()->get_method_id("add", "(II)I"));

	auto field = test_class::to_reflected_static_field<"staticIntField", jint>();
	name = field->call_method<"getName", jni::ref<jni::string>()>();
	CHECK(name == "staticIntField");
}

TEST_CASE("instance_of and casts", "[object]")
{
	jni::ref<> obj = jni::string::new_object("x");

	CHECK(obj->is_instance_of<jni::string>());
	CHECK(obj->is_instance_of<jni::object<"java/lang/CharSequence">>());
	CHECK_FALSE(obj->is_instance_of<test_class>());

	CHECK(obj->instance_of<jni::string>());
	CHECK_FALSE(obj->instance_of<test_class>());

	CHECK(obj->get_object_class() == jni::string::class_object());

	CHECK(jni::dynamic_pointer_cast<jni::string>(obj));
	CHECK_FALSE(jni::dynamic_pointer_cast<test_class>(obj));

	auto s = jni::static_pointer_cast<jni::string>(obj);
	CHECK(s->length() == 1);
}
