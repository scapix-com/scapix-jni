#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <concepts>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <scapix/jni/native_method.h>
#include <scapix/jni/array.h>
#include <scapix/jni/string.h>
#include <scapix/jni/convert.h>
#include <scapix/jni/vm_exception.h>

namespace jni = scapix::jni;
using Catch::Matchers::ContainsSubstring;

// tests/java/com/scapix/test/NativeTest.java

namespace native_method_test {

using native_test = jni::object<"com/scapix/test/NativeTest">;

// C++ object associated with NativeTest instance (NativeTest.ptr field).

struct counter
{
	jint add(jint a) { return total += a; }
	jint get() const { return total; }
	void set(jint v) noexcept { total = v; }
	jint get_noexcept() const noexcept { return total; }
	std::string name() const { return "counter " + std::to_string(total); }

	jint mul(jint v) { return total * v; }
	jlong mul(jlong v) { return total * v; }

	std::string format(int v) const { return "int " + std::to_string(total + v); }
	std::string format(double v) const { return "double " + std::to_string(total + v); }

	void fail() { throw std::logic_error("counter failed"); }

	static jint static_member(jni::ref<jni::class_>, jint v) { return v * 10; }

	jint total = 0;
};

} // namespace native_method_test

namespace scapix::jni {

template <std::same_as<native_method_test::counter> T>
T& convert_this(ref<native_method_test::native_test> x)
{
	return *reinterpret_cast<T*>(x->get_field<"ptr", jlong>());
}

} // namespace scapix::jni

namespace native_method_test {

namespace {

// static, ref<class_> first parameter, JNI signature deduced

jint add(jni::ref<jni::class_>, jint a, jint b) { return a + b; }
jni::ref<jni::string> null_string(jni::ref<jni::class_>) { return {}; }

// static, primitive types

jboolean not_boolean(jni::ref<jni::class_>, jboolean v) { return !v; }
jbyte inc_byte(jni::ref<jni::class_>, jbyte v) { return v + 1; }
jchar inc_char(jni::ref<jni::class_>, jchar v) { return v + 1; }
jshort inc_short(jni::ref<jni::class_>, jshort v) { return v + 1; }
jint inc_int(jni::ref<jni::class_>, jint v) { return v + 1; }
jlong inc_long(jni::ref<jni::class_>, jlong v) { return v + 1; }
jfloat half_float(jni::ref<jni::class_>, jfloat v) { return v / 2; }
jdouble half_double(jni::ref<jni::class_>, jdouble v) { return v / 2; }

jdouble sum_all(jni::ref<jni::class_>, jboolean z, jbyte b, jchar c, jshort s, jint i, jlong j, jfloat f, jdouble d)
{
	return z + b + c + s + i + j + f + d;
}

jint static_value = 0;
void set_static(jni::ref<jni::class_>, jint v) { static_value = v; }
jint get_static(jni::ref<jni::class_>) { return static_value; }

// static, no ref<class_> parameter, JNI signature specified

jint mul(jint a, jint b) { return a * b; }

// static, C++ types

std::string concat(jni::ref<jni::class_>, std::string a, std::string b) { return a + b; }
std::string concat_ref(const std::string& a, const std::string& b) { return a + "+" + b; }
std::vector<int> range(jni::ref<jni::class_>, int n) { std::vector<int> v(n); for (int i = 0; i < n; ++i) v[i] = i; return v; }
int sum(jni::ref<jni::class_>, std::vector<int> v) { int s = 0; for (int i : v) s += i; return s; }
std::vector<std::string> repeat(jni::ref<jni::class_>, std::string s, int n) { return std::vector<std::string>(n, s); }
bool is_even(jni::ref<jni::class_>, int v) { return v % 2 == 0; }

enum class color : std::int32_t { red, green, blue };
color next_color(jni::ref<jni::class_>, color c) { return color((std::int32_t(c) + 1) % 3); }

// static, overloaded C++ functions

jint twice(jint v) { return v * 2; }
jlong twice(jlong v) { return v * 2; }

jint neg(jni::ref<jni::class_>, jint v) { return -v; }
jlong neg(jni::ref<jni::class_>, jlong v) { return -v; }

std::string label(int v) { return "int " + std::to_string(v); }
std::string label(double v) { return "double " + std::to_string(v); }

std::string describe(jni::ref<jni::class_>, int v) { return "int " + std::to_string(v); }
std::string describe(jni::ref<jni::class_>, double v) { return "double " + std::to_string(v); }

// instance, ref<NativeTest> first parameter

jint plus_value(jni::ref<native_test> self, jint a) { return self->get_field<"value", jint>() + a; }
jni::ref<jni::string> greet(jni::ref<native_test>, jni::ref<jni::string> name) { return jni::string::new_object(("hello " + std::string(name)).c_str()); }
std::string greet_cpp(jni::ref<native_test> self, const std::string& name) { return name + " " + std::to_string(self->get_field<"value", jint>()); }

// exceptions

void throw_cpp(jni::ref<jni::class_>, std::string message) { throw std::runtime_error(message); }
void throw_java(jni::ref<jni::class_>, jni::ref<jni::string> message) { native_test::call_static_method<"throwIllegalState", void(jni::ref<jni::string>)>(message); }

// android @CriticalNative (not supported by desktop JVM, compile time checks only)

jint critical_add(jint a, jint b) { return a + b; }

// native_method_info

template <auto Info, typename JniType, typename CppType, bool AndroidCriticalNative = false>
constexpr bool check_info =
	std::is_same_v<typename decltype(Info)::jni_type, JniType> &&
	std::is_same_v<typename decltype(Info)::cpp_type, CppType> &&
	Info.android_critical_native == AndroidCriticalNative;

using jstring_ = jni::ref<jni::string>;
using class_ = jni::ref<jni::class_>;

// no types specified

constexpr auto m_add = jni::native_method("add", &add);
constexpr auto m_null_string = jni::native_method("nullString", &null_string);
constexpr auto m_not_boolean = jni::native_method("notBoolean", &not_boolean);
constexpr auto m_inc_byte = jni::native_method("incByte", &inc_byte);
constexpr auto m_inc_char = jni::native_method("incChar", &inc_char);
constexpr auto m_inc_short = jni::native_method("incShort", &inc_short);
constexpr auto m_inc_int = jni::native_method("incInt", &inc_int);
constexpr auto m_inc_long = jni::native_method("incLong", &inc_long);
constexpr auto m_half_float = jni::native_method("halfFloat", &half_float);
constexpr auto m_half_double = jni::native_method("halfDouble", &half_double);
constexpr auto m_sum_all = jni::native_method("sumAll", &sum_all);
constexpr auto m_set_static = jni::native_method("setStatic", &set_static);
constexpr auto m_get_static = jni::native_method("getStatic", &get_static);
constexpr auto m_static_member = jni::native_method("staticMember", &counter::static_member);
constexpr auto m_plus_value = jni::native_method("plusValue", &plus_value);
constexpr auto m_greet = jni::native_method("greet", &greet);
constexpr auto m_counter_add = jni::native_method("counterAdd", &counter::add);
constexpr auto m_counter_get = jni::native_method("counterGet", &counter::get);
constexpr auto m_counter_set = jni::native_method("counterSet", &counter::set);
constexpr auto m_counter_get_noexcept = jni::native_method("counterGetNoexcept", &counter::get_noexcept);
constexpr auto m_counter_throw = jni::native_method("counterThrow", &counter::fail);
constexpr auto m_throw_java = jni::native_method("throwJava", &throw_java);

static_assert(check_info<m_add, jint(jint, jint), jint(*)(class_, jint, jint)>);
static_assert(check_info<m_null_string, jstring_(), jstring_(*)(class_)>);
static_assert(check_info<m_sum_all, jdouble(jboolean, jbyte, jchar, jshort, jint, jlong, jfloat, jdouble), decltype(&sum_all)>);
static_assert(check_info<m_set_static, void(jint), void(*)(class_, jint)>);
static_assert(check_info<m_static_member, jint(jint), jint(*)(class_, jint)>);
static_assert(check_info<m_plus_value, jint(jint), jint(*)(jni::ref<native_test>, jint)>);
static_assert(check_info<m_greet, jstring_(jstring_), jstring_(*)(jni::ref<native_test>, jstring_)>);
static_assert(check_info<m_counter_add, jint(jint), jint(counter::*)(jint)>);
static_assert(check_info<m_counter_get, jint(), jint(counter::*)() const>);
static_assert(check_info<m_counter_set, void(jint), void(counter::*)(jint) noexcept>);
static_assert(check_info<m_counter_get_noexcept, jint(), jint(counter::*)() const noexcept>);
static_assert(check_info<m_counter_throw, void(), void(counter::*)()>);

// only JniType specified

constexpr auto m_mul = jni::native_method<jint(jint, jint)>("mul", &mul);
constexpr auto m_concat = jni::native_method<jstring_(jstring_, jstring_)>("concat", &concat);
constexpr auto m_concat_ref = jni::native_method<jstring_(jstring_, jstring_)>("concatRef", &concat_ref);
constexpr auto m_range = jni::native_method<jni::ref<jni::array<jint>>(jint)>("range", &range);
constexpr auto m_sum = jni::native_method<jint(jni::ref<jni::array<jint>>)>("sum", &sum);
constexpr auto m_repeat = jni::native_method<jni::ref<jni::array<jni::string>>(jstring_, jint)>("repeat", &repeat);
constexpr auto m_is_even = jni::native_method<jboolean(jint)>("isEven", &is_even);
constexpr auto m_next_color = jni::native_method<jint(jint)>("nextColor", &next_color);
constexpr auto m_twice_int = jni::native_method<jint(jint)>("twiceInt", &twice);
constexpr auto m_twice_long = jni::native_method<jlong(jlong)>("twiceLong", &twice);
constexpr auto m_neg_int = jni::native_method<jint(jint)>("negInt", &neg);
constexpr auto m_neg_long = jni::native_method<jlong(jlong)>("negLong", &neg);
constexpr auto m_greet_cpp = jni::native_method<jstring_(jstring_)>("greetCpp", &greet_cpp);
constexpr auto m_counter_name = jni::native_method<jstring_()>("counterName", &counter::name);
constexpr auto m_counter_mul_int = jni::native_method<jint(jint)>("counterMulInt", &counter::mul);
constexpr auto m_counter_mul_long = jni::native_method<jlong(jlong)>("counterMulLong", &counter::mul);
constexpr auto m_throw_cpp = jni::native_method<void(jstring_)>("throwCpp", &throw_cpp);

static_assert(check_info<m_mul, jint(jint, jint), jint(*)(jint, jint)>);
static_assert(check_info<m_concat, jstring_(jstring_, jstring_), std::string(*)(class_, std::string, std::string)>);
static_assert(check_info<m_concat_ref, jstring_(jstring_, jstring_), std::string(*)(const std::string&, const std::string&)>);
static_assert(check_info<m_twice_int, jint(jint), jint(*)(jint)>);
static_assert(check_info<m_twice_long, jlong(jlong), jlong(*)(jlong)>);
static_assert(check_info<m_neg_int, jint(jint), jint(*)(class_, jint)>);
static_assert(check_info<m_neg_long, jlong(jlong), jlong(*)(class_, jlong)>);
static_assert(check_info<m_greet_cpp, jstring_(jstring_), std::string(*)(jni::ref<native_test>, const std::string&)>);
static_assert(check_info<m_counter_name, jstring_(), std::string(counter::*)() const>);
static_assert(check_info<m_counter_mul_int, jint(jint), jint(counter::*)(jint)>);
static_assert(check_info<m_counter_mul_long, jlong(jlong), jlong(counter::*)(jlong)>);

// both types specified - CppType selects overload

constexpr auto m_label_int = jni::native_method<jstring_(jint), std::string(int)>("labelInt", &label);
constexpr auto m_label_double = jni::native_method<jstring_(jdouble), std::string(double)>("labelDouble", &label);
constexpr auto m_describe_int = jni::native_method<jstring_(jint), std::string(int)>("describeInt", &describe);
constexpr auto m_describe_double = jni::native_method<jstring_(jdouble), std::string(double)>("describeDouble", &describe);
constexpr auto m_counter_format_int = jni::native_method<jstring_(jint), std::string(int) const>("counterFormatInt", &counter::format);
constexpr auto m_counter_format_double = jni::native_method<jstring_(jdouble), std::string(double) const>("counterFormatDouble", &counter::format);

static_assert(check_info<m_label_int, jstring_(jint), std::string(*)(int)>);
static_assert(check_info<m_label_double, jstring_(jdouble), std::string(*)(double)>);
static_assert(check_info<m_describe_int, jstring_(jint), std::string(*)(class_, int)>);
static_assert(check_info<m_describe_double, jstring_(jdouble), std::string(*)(class_, double)>);
static_assert(check_info<m_counter_format_int, jstring_(jint), std::string(counter::*)(int) const>);
static_assert(check_info<m_counter_format_double, jstring_(jdouble), std::string(counter::*)(double) const>);

// android_critical_native_method

constexpr auto m_critical_add = jni::android_critical_native_method("criticalAdd", &critical_add);

static_assert(check_info<m_critical_add, jint(jint, jint), jint(*)(jint, jint), true>);

using natives = jni::native_methods
<
	native_test::class_name,

	m_add, m_null_string,
	m_not_boolean, m_inc_byte, m_inc_char, m_inc_short, m_inc_int, m_inc_long, m_half_float, m_half_double, m_sum_all,
	m_set_static, m_get_static,
	m_mul,
	m_concat, m_concat_ref, m_range, m_sum, m_repeat, m_is_even, m_next_color,
	m_twice_int, m_twice_long, m_neg_int, m_neg_long,
	m_label_int, m_label_double, m_describe_int, m_describe_double,
	m_static_member,
	m_plus_value, m_greet, m_greet_cpp,
	m_counter_add, m_counter_get, m_counter_set, m_counter_get_noexcept, m_counter_name,
	m_counter_mul_int, m_counter_mul_long, m_counter_format_int, m_counter_format_double,
	m_throw_cpp, m_throw_java, m_counter_throw
>;

void register_natives()
{
	static const bool registered = (natives::register_(), true);
	(void)registered;
}

// Java NativeTest object with associated C++ counter.

class instance
{
public:

	instance(jint value, jint total) : obj(jni::new_object<native_test, void(jint)>(value))
	{
		register_natives();
		c.total = total;
		obj->set_field<"ptr", jlong>(reinterpret_cast<jlong>(&c));
	}

	instance(const instance&) = delete;
	instance& operator=(const instance&) = delete;

	counter c;
	jni::ref<native_test> obj;
};

} // namespace

TEST_CASE("native method: static, JNI types", "[native]")
{
	register_natives();

	CHECK(native_test::call_static_method<"add", jint(jint, jint)>(1, 2) == 3);
	CHECK_FALSE(native_test::call_static_method<"nullString", jstring_()>());

	native_test::call_static_method<"setStatic", void(jint)>(77);
	CHECK(static_value == 77);
	CHECK(native_test::call_static_method<"getStatic", jint()>() == 77);
}

TEST_CASE("native method: static, primitive types", "[native]")
{
	register_natives();

	CHECK(native_test::call_static_method<"notBoolean", jboolean(jboolean)>(JNI_TRUE) == JNI_FALSE);
	CHECK(native_test::call_static_method<"notBoolean", jboolean(jboolean)>(JNI_FALSE) == JNI_TRUE);
	CHECK(native_test::call_static_method<"incByte", jbyte(jbyte)>(jbyte(-2)) == -1);
	CHECK(native_test::call_static_method<"incChar", jchar(jchar)>(jchar(u'a')) == u'b');
	CHECK(native_test::call_static_method<"incShort", jshort(jshort)>(jshort(1000)) == 1001);
	CHECK(native_test::call_static_method<"incInt", jint(jint)>(-1) == 0);
	CHECK(native_test::call_static_method<"incLong", jlong(jlong)>(jlong(1) << 40) == (jlong(1) << 40) + 1);
	CHECK(native_test::call_static_method<"halfFloat", jfloat(jfloat)>(3.0f) == 1.5f);
	CHECK(native_test::call_static_method<"halfDouble", jdouble(jdouble)>(5.0) == 2.5);

	CHECK(native_test::call_static_method<"sumAll", jdouble(jboolean, jbyte, jchar, jshort, jint, jlong, jfloat, jdouble)>
		(JNI_TRUE, jbyte(2), jchar(3), jshort(4), jint(5), jlong(6), jfloat(7.5f), jdouble(8.25)) == 36.75);
}

TEST_CASE("native method: static without ref<class_> parameter", "[native]")
{
	register_natives();

	CHECK(native_test::call_static_method<"mul", jint(jint, jint)>(6, 7) == 42);
	CHECK(native_test::call_static_method<"staticMember", jint(jint)>(4) == 40);
}

TEST_CASE("native method: static, C++ types", "[native]")
{
	register_natives();

	std::string s = native_test::call_static_method<"concat", jstring_(jstring_, jstring_)>("abc", "def");
	CHECK(s == "abcdef");

	s = native_test::call_static_method<"concatRef", jstring_(jstring_, jstring_)>("abc", "def");
	CHECK(s == "abc+def");

	std::vector<int> r = native_test::call_static_method<"range", jni::ref<jni::array<jint>>(jint)>(4);
	CHECK(r == std::vector<int>{ 0, 1, 2, 3 });

	auto a = jni::convert_jni<jni::ref<jni::array<jint>>>(std::vector<int>{ 1, 2, 3, 4 });
	CHECK(native_test::call_static_method<"sum", jint(jni::ref<jni::array<jint>>)>(a) == 10);

	std::vector<std::string> v = native_test::call_static_method<"repeat", jni::ref<jni::array<jni::string>>(jstring_, jint)>("x", 3);
	CHECK(v == std::vector<std::string>{ "x", "x", "x" });

	CHECK(native_test::call_static_method<"isEven", jboolean(jint)>(4) == JNI_TRUE);
	CHECK(native_test::call_static_method<"isEven", jboolean(jint)>(5) == JNI_FALSE);

	CHECK(native_test::call_static_method<"nextColor", jint(jint)>(jint(color::green)) == jint(color::blue));
	CHECK(native_test::call_static_method<"nextColor", jint(jint)>(jint(color::blue)) == jint(color::red));
}

TEST_CASE("native method: static, overloaded C++ functions", "[native]")
{
	register_natives();

	SECTION("selected by JniType, without ref<class_> parameter")
	{
		CHECK(native_test::call_static_method<"twiceInt", jint(jint)>(21) == 42);
		CHECK(native_test::call_static_method<"twiceLong", jlong(jlong)>(jlong(1) << 40) == jlong(1) << 41);
	}

	SECTION("selected by JniType, with ref<class_> parameter")
	{
		CHECK(native_test::call_static_method<"negInt", jint(jint)>(5) == -5);
		CHECK(native_test::call_static_method<"negLong", jlong(jlong)>(jlong(1) << 40) == -(jlong(1) << 40));
	}

	SECTION("selected by CppType, without ref<class_> parameter")
	{
		std::string s = native_test::call_static_method<"labelInt", jstring_(jint)>(3);
		CHECK(s == "int 3");
		s = native_test::call_static_method<"labelDouble", jstring_(jdouble)>(0.5);
		CHECK(s == "double 0.500000");
	}

	SECTION("selected by CppType, with ref<class_> parameter")
	{
		std::string s = native_test::call_static_method<"describeInt", jstring_(jint)>(3);
		CHECK(s == "int 3");
		s = native_test::call_static_method<"describeDouble", jstring_(jdouble)>(0.5);
		CHECK(s == "double 0.500000");
	}
}

TEST_CASE("native method: instance, ref<T> first parameter", "[native]")
{
	instance i(7, 0);

	CHECK(i.obj->call_method<"plusValue", jint(jint)>(3) == 10);

	std::string s = i.obj->call_method<"greet", jstring_(jstring_)>("world");
	CHECK(s == "hello world");

	s = i.obj->call_method<"greetCpp", jstring_(jstring_)>("value");
	CHECK(s == "value 7");
}

TEST_CASE("native method: instance, C++ member function", "[native]")
{
	instance i(0, 10);

	SECTION("plain")
	{
		CHECK(i.obj->call_method<"counterAdd", jint(jint)>(5) == 15);
		CHECK(i.c.total == 15);
	}

	SECTION("const")
	{
		CHECK(i.obj->call_method<"counterGet", jint()>() == 10);
	}

	SECTION("noexcept")
	{
		i.obj->call_method<"counterSet", void(jint)>(33);
		CHECK(i.c.total == 33);
	}

	SECTION("const noexcept")
	{
		CHECK(i.obj->call_method<"counterGetNoexcept", jint()>() == 10);
	}

	SECTION("C++ types")
	{
		std::string s = i.obj->call_method<"counterName", jstring_()>();
		CHECK(s == "counter 10");
	}

	SECTION("overload selected by JniType")
	{
		CHECK(i.obj->call_method<"counterMulInt", jint(jint)>(3) == 30);
		CHECK(i.obj->call_method<"counterMulLong", jlong(jlong)>(jlong(1) << 40) == 10 * (jlong(1) << 40));
	}

	SECTION("overload selected by CppType")
	{
		std::string s = i.obj->call_method<"counterFormatInt", jstring_(jint)>(1);
		CHECK(s == "int 11");
		s = i.obj->call_method<"counterFormatDouble", jstring_(jdouble)>(0.5);
		CHECK(s == "double 10.500000");
	}

	SECTION("each Java object uses its own C++ object")
	{
		instance j(0, 100);
		CHECK(i.obj->call_method<"counterGet", jint()>() == 10);
		CHECK(j.obj->call_method<"counterGet", jint()>() == 100);
	}
}

TEST_CASE("native method: called from Java", "[native]")
{
	instance i(7, 1);

	CHECK(native_test::call_static_method<"callAdd", jint(jint, jint)>(20, 22) == 42);
	CHECK(i.obj->call_method<"callCounterAdd", jint(jint)>(2) == 3);

	std::string s = i.obj->call_method<"callGreet", jstring_(jstring_)>("java");
	CHECK(s == "hello java");
}

TEST_CASE("native method: C++ exception is visible to Java as com.scapix.NativeException", "[native][exception]")
{
	register_natives();

	std::string s = native_test::call_static_method<"catchThrowCpp", jstring_(jstring_)>("thrown from native method");
	CHECK(s == "com.scapix.NativeException: thrown from native method");
}

TEST_CASE("native method: C++ exception tunnels through Java back to C++", "[native][exception]")
{
	SECTION("static")
	{
		register_natives();
		CHECK_THROWS_AS((native_test::call_static_method<"callThrowCpp", void(jstring_)>("thrown from native method")), std::runtime_error);
		CHECK_THROWS_WITH((native_test::call_static_method<"callThrowCpp", void(jstring_)>("thrown from native method")), "thrown from native method");
		CHECK_THROWS_WITH((native_test::call_static_method<"throwCpp", void(jstring_)>("called directly")), "called directly");
	}

	SECTION("member function")
	{
		instance i(0, 0);
		CHECK_THROWS_AS((i.obj->call_method<"counterThrow", void()>()), std::logic_error);
	}

	CHECK_FALSE(jni::detail::env()->ExceptionCheck());
}

TEST_CASE("native method: Java exception propagates through native method", "[native][exception]")
{
	register_natives();

	std::string s = native_test::call_static_method<"catchThrowJava", jstring_(jstring_)>("bad state");
	CHECK(s == "java.lang.IllegalStateException: bad state");

	try
	{
		native_test::call_static_method<"throwJava", void(jstring_)>("bad state");
		FAIL("exception expected");
	}
	catch (const jni::vm_exception& e)
	{
		CHECK(e.instance_of<jni::object<"java/lang/IllegalStateException", jni::throwable>>());
		CHECK_THAT(e.what(), ContainsSubstring("bad state"));
	}

	CHECK_FALSE(jni::detail::env()->ExceptionCheck());
}

TEST_CASE("native method: register_ can be called more than once", "[native]")
{
	natives::register_();
	natives::register_();
	CHECK(native_test::call_static_method<"add", jint(jint, jint)>(2, 3) == 5);
}

} // namespace native_method_test
