#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <scapix/jni/ref.h>
#include <scapix/jni/string.h>
#include <scapix/jni/compare.h>
#include <scapix/jni/convert.h>
#include <scapix/jni/local_frame.h>
#include <scapix/jni/monitor.h>
#include <scapix/jni/env.h>
#include <scapix/jni/class_loader.h>

#include "test_class.h"

using namespace test;

namespace {

jobjectRefType ref_type(jobject obj)
{
	return jni::detail::env()->GetObjectRefType(obj);
}

bool holds_lock(jni::ref<> obj)
{
	return jni::object<"java/lang/Thread">::call_static_method<"holdsLock", jboolean(jni::ref<>)>(obj);
}

} // namespace

TEST_CASE("ref scopes", "[ref]")
{
	jni::local_ref<jni::string> local = jni::string::new_object("abc");
	REQUIRE(local);
	CHECK(local.get_scope() == jni::scope::local);
	CHECK(ref_type(local.handle()) == JNILocalRefType);

	SECTION("global_ref")
	{
		jni::global_ref<jni::string> global = local;
		CHECK(global.get_scope() == jni::scope::global);
		CHECK(ref_type(global.handle()) == JNIGlobalRefType);
		CHECK(global == local);
		CHECK(global.handle() != local.handle());
	}

	SECTION("weak_ref")
	{
		jni::weak_ref<jni::string> weak = local;
		CHECK(ref_type(weak.handle()) == JNIWeakGlobalRefType);
		CHECK(weak == local);
	}

	SECTION("generic ref copy does not own")
	{
		jni::ref<jni::string> r = local;
		CHECK(r.get_scope() == jni::scope::generic);
		CHECK(r.handle() == local.handle());
	}

	SECTION("generic ref move takes ownership")
	{
		auto handle = local.handle();
		jni::ref<jni::string> r = std::move(local);
		CHECK_FALSE(local);
		CHECK(r.get_scope() == jni::scope::local);
		CHECK(r.handle() == handle);
	}

	SECTION("move between scopes creates new ref")
	{
		// source keeps (and later deletes) its local ref
		jni::global_ref<jni::string> global = std::move(local);
		CHECK(ref_type(global.handle()) == JNIGlobalRefType);
		CHECK(global == local);
	}

	SECTION("conversion to base")
	{
		jni::local_ref<> obj = local;
		CHECK(obj == local);
	}

	SECTION("reset, release, swap")
	{
		jni::local_ref<jni::string> other;
		swap(local, other);
		CHECK_FALSE(local);
		CHECK(other);

		other.reset();
		CHECK_FALSE(other);

		auto r = jni::string::new_object("x");
		jstring h = r.release();
		CHECK_FALSE(r);
		jni::detail::env()->DeleteLocalRef(h);
	}
}

TEST_CASE("ref equality is identity", "[ref]")
{
	auto a = jni::string::new_object("same");
	auto b = jni::string::new_object("same");

	CHECK(a == a);
	CHECK_FALSE(a == b);
	CHECK(jni::ref_value_equal<jni::string>()(a, b));
	CHECK(jni::ref_value_hash<jni::string>()(a) == jni::ref_value_hash<jni::string>()(b));
	CHECK(jni::local_ref<jni::string>() == jni::local_ref<jni::string>());
}

TEST_CASE("ref in std::map (Comparable)", "[ref][compare]")
{
	std::map<jni::global_ref<jni::string>, int> map;

	map.emplace(jni::string::new_object("b"), 2);
	map.emplace(jni::string::new_object("c"), 3);
	map.emplace(jni::string::new_object("a"), 1);
	map.emplace(jni::string::new_object("a"), 4); // duplicate by value

	REQUIRE(map.size() == 3);
	CHECK(std::string(jni::ref<jni::string>(map.begin()->first)) == "a");
	CHECK(map.find(jni::string::new_object("c"))->second == 3);
}

TEST_CASE("ref in std::unordered_map", "[ref][compare]")
{
	SECTION("identity")
	{
		std::unordered_map<jni::global_ref<jni::string>, int> map;
		auto key = jni::string::new_object("key");
		map.emplace(key, 1);
		map.emplace(jni::string::new_object("key"), 2);

		CHECK(map.size() == 2);
		CHECK(map.find(key)->second == 1);
		CHECK(map.find(jni::string::new_object("key")) == map.end());
	}

	SECTION("value")
	{
		std::unordered_map<jni::global_ref<jni::string>, int, jni::ref_value_hash<jni::string>, jni::ref_value_equal<jni::string>> map;
		map.emplace(jni::string::new_object("key"), 1);
		map.emplace(jni::string::new_object("key"), 2);

		CHECK(map.size() == 1);
		CHECK(map.find(jni::string::new_object("key"))->second == 1);
	}
}

TEST_CASE("local_frame", "[ref]")
{
	SECTION("pop")
	{
		jni::local_frame frame(4);
		CHECK(frame);

		for (int i = 0; i < 100; ++i)
			jni::string::new_object("in frame").release(); // freed by frame

		frame.pop();
		CHECK_FALSE(frame);

		frame.pop(); // second pop is a no-op
		CHECK_FALSE(frame);
	}

	SECTION("pop with result")
	{
		jni::local_ref<jni::string> result;

		{
			jni::local_frame frame(4);
			result = frame.pop(jni::string::new_object("result"));
			CHECK_FALSE(frame);
		}

		REQUIRE(result);
		CHECK(std::string(jni::ref<jni::string>(result)) == "result");
	}
}

TEST_CASE("monitor", "[ref]")
{
	auto obj = jni::new_object<jni::object<>, void()>();
	CHECK_FALSE(holds_lock(obj));

	{
		jni::monitor m(obj);
		CHECK(m);
		CHECK(holds_lock(obj));

		jni::monitor m2 = std::move(m);
		CHECK_FALSE(m);
		CHECK(holds_lock(obj));
	}

	CHECK_FALSE(holds_lock(obj));
}

TEST_CASE("native thread is attached automatically", "[thread]")
{
	std::string result;
	bool app_class_found = false;
	bool attached = false;

	std::thread([&]
	{
		jni::ref<jni::string> s = std::string("from thread");
		result = s;

		attached = jni::get_env() == JNI_OK;

#ifdef SCAPIX_JNI_CACHE_CLASS_LOADER
		// FindClass() from native thread uses system class loader,
		// cached class loader must find application classes as well.
		app_class_found = bool(jni::class_loader::find_class("com.scapix.test.TestClass"));
#else
		app_class_found = true;
#endif
	}).join();

	CHECK(result == "from thread");
	CHECK(attached);
	CHECK(app_class_found);
}
