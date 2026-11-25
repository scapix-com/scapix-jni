#include <catch2/catch_test_macros.hpp>

#include <scapix/jni/object_base.h>
#include <scapix/jni/string.h>
#include <scapix/jni/generics.h>

// Classes declared like generated bindings.

namespace test_object_base {

class Object;
class Number;
class Integer;
template <typename E> class ArrayList;

} // namespace test_object_base

template <>
struct scapix::java_api::object_traits<test_object_base::Object>
{
	static constexpr fixed_string class_name = "java/lang/Object";
	using base_classes = std::tuple<>;
};

template <>
struct scapix::java_api::object_traits<test_object_base::Number>
{
	static constexpr fixed_string class_name = "java/lang/Number";
	using base_classes = std::tuple<test_object_base::Object>;
};

template <>
struct scapix::java_api::object_traits<test_object_base::Integer>
{
	static constexpr fixed_string class_name = "java/lang/Integer";
	using base_classes = std::tuple<test_object_base::Number>;
};

template <typename E>
struct scapix::java_api::object_traits<test_object_base::ArrayList<E>>
{
	static constexpr fixed_string class_name = "java/util/ArrayList";
	using base_classes = std::tuple<test_object_base::Object>;
};

namespace test_object_base {

namespace jni = scapix::jni;

class Object : public jni::object_base<"java/lang/Object">
{
public:

	static jni::ref<Object> new_object() { return object_base::new_object(); }
	jint hashCode() { return call_method<"hashCode", jint>(); }
};

class Number : public jni::object_base<"java/lang/Number", Object>
{
public:

	jint intValue() { return call_method<"intValue", jint>(); }
};

class Integer : public jni::object_base<"java/lang/Integer", Number>
{
public:

	static jni::ref<Integer> new_object(jint value) { return object_base::new_object(value); }
	static jni::ref<Integer> valueOf(jint i) { return call_static_method<"valueOf", jni::ref<Integer>>(i); }
	static jint MAX_VALUE() { return get_static_field<"MAX_VALUE", jint>(); }
};

template <typename E>
class ArrayList : public jni::object_base<"java/util/ArrayList", Object>
{
	using object_base = jni::object_base<"java/util/ArrayList">;

public:

	static jni::ref<ArrayList<E>> new_object() { return object_base::new_object(); }
	static jni::ref<ArrayList<E>> new_object(jint initialCapacity) { return object_base::new_object(initialCapacity); }
	jint size() { return object_base::call_method<"size", jint>(); }
	jboolean add(jni::ref<E> e) { return object_base::call_method<"add", jboolean>(jni::ref<jni::erased<E>>(e)); }
	jni::ref<E> get(jint index) { return object_base::call_method<"get", jni::ref<jni::erased<E>>>(index); }
};

// Signature-deducing members are not accessible outside of generated classes.

template <typename T, typename ...Args>
constexpr bool has_new_object = requires (Args... args) { T::new_object(args...); };

static_assert(!has_new_object<jni::object<"[I">, jint>);
static_assert(!has_new_object<jni::object<"java/lang/String">, jni::ref<jni::string>>);
static_assert(!has_new_object<jni::object_base<"java/lang/Integer">, jint>);
static_assert(!has_new_object<jni::object_base<"java/lang/Integer", Number>, jint>);
static_assert(has_new_object<Integer, jint>);

} // namespace test_object_base

using namespace test_object_base;

TEST_CASE("object_base", "[object_base]")
{
	SECTION("no base classes")
	{
		auto obj = Object::new_object();
		REQUIRE(obj);
		CHECK(obj->hashCode() == obj->hashCode());
	}

	SECTION("base classes")
	{
		auto i = Integer::new_object(42);
		CHECK(jni::new_object<Integer>(43)->intValue() == 43);
		REQUIRE(i);
		CHECK(i->intValue() == 42);
		CHECK(Integer::valueOf(7)->intValue() == 7);
		CHECK(Integer::MAX_VALUE() == 2147483647);
	}

	SECTION("generic class")
	{
		auto list = ArrayList<Integer>::new_object(10);
		REQUIRE(list);
		CHECK(list->size() == 0);
		CHECK(list->add(Integer::new_object(5)));
		CHECK(list->size() == 1);
		CHECK(list->get(0)->intValue() == 5);
		CHECK(ArrayList<Integer>::new_object()->size() == 0);
	}
}
