/*
	scapix/jni/convert.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_CONVERT_H
#define SCAPIX_JNI_CONVERT_H

#include <algorithm>
#include <concepts>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <scapix/core/immortal.h>
#include <scapix/core/meta/for_each.h>
#include <scapix/jni/ref.h>
#include <scapix/jni/string.h>
#include <scapix/jni/array.h>
#include <scapix/jni/function.h>
#include <scapix/jni/struct.h>
#include <scapix/jni/cast.h>
#include <scapix/jni/range.h>
#include <scapix/jni/bridge/function.h>

namespace scapix::jni {

template <typename J, typename T>
struct convert<ref<J>, std::shared_ptr<T>>
{
	// null handled by convert_cpp

	static std::shared_ptr<T> cpp(ref<J> value)
	{
		return std::make_shared<T>(convert_cpp<T>(value));
	}

	static ref<J> jni(const std::shared_ptr<T>& value)
	{
		if (!value)
			return nullptr;

		return convert_jni<ref<J>>(*value);
	}
};

template <primitive Jni, typename Cpp>
	requires (std::is_integral_v<Jni> && std::is_integral_v<Cpp> && sizeof(Jni) == sizeof(Cpp)) || std::is_same_v<Jni, Cpp>
struct convert<Jni, Cpp>
{
	static Cpp cpp(Jni v)
	{
		return v;
	}

	static Jni jni(Cpp v)
	{
		return v;
	}
};

template <typename Jni, typename Cpp>
	requires std::is_integral_v<Jni> && std::is_enum_v<Cpp> && (sizeof(Jni) == sizeof(Cpp))
struct convert<Jni, Cpp>
{
	static Cpp cpp(Jni value)
	{
		return static_cast<Cpp>(value);
	}

	static Jni jni(Cpp value)
	{
		return static_cast<Jni>(value);
	}
};

template <typename J, typename Cpp>
	requires std::is_enum_v<Cpp>
struct convert<ref<J>, Cpp>
{
	using underlying = std::underlying_type_t<Cpp>;

	static Cpp cpp(ref<J> value)
	{
		return static_cast<Cpp>(convert_cpp<underlying>(value));
	}

	static ref<J> jni(Cpp value)
	{
		return convert_jni<ref<J>>(static_cast<underlying>(value));
	}
};

#ifdef SCAPIX_JNI_STANDARD_UTF8

struct convert_string
{
	using charset = object<"java/nio/charset/Charset">;
	using standard_charsets = object<"java/nio/charset/StandardCharsets">;

	static ref<charset> utf8_charset()
	{
		static const immortal<global_ref<charset>> ch = standard_charsets::get_static_field<"UTF_8", ref<charset>>();
		return ch;
	}

	static std::string cpp(ref<string> obj)
	{
		auto bytes = obj->call_method<"getBytes", ref<array<jbyte>>(ref<charset>)>(utf8_charset());
		std::string str(bytes->size(), char());
		bytes->get_region(0, static_cast<jsize>(str.size()), reinterpret_cast<jbyte*>(str.data()));

		return str;
	}

	static ref<string> jni(std::string_view str)
	{
		auto bytes = new_object<array<jbyte>>(static_cast<jsize>(str.size()));
		bytes->set_region(0, static_cast<jsize>(str.size()), reinterpret_cast<const jbyte*>(str.data()));

		return new_object<string, void(ref<array<jbyte>>, ref<charset>)>(bytes, utf8_charset());
	}
};

#else // Modified UTF-8

struct convert_string
{
	static std::string cpp(ref<string> obj)
	{
		std::string str(obj->length<char>(), char());
		obj->get_region(0, obj->length(), str.data());
		return str;
	}

	// NewStringUTF requires null terminated string

//	static ref<string> jni(std::cstring_view str)
//	{
//		return new_object<string>(str);
//	}

	static ref<string> jni(const char* str)
	{
		return new_object<string>(str);
	}

	static ref<string> jni(const std::string& str)
	{
		return jni(str.c_str());
	}
};

#endif

template <object_convertible_to<string> J, std::convertible_to<std::string> Cpp>
struct convert<ref<J>, Cpp> : convert_string
{
};

template <fixed_string ClassName, typename CppPrimitive, typename JniPrimitive, fixed_string MethodName>
struct convert_primitive_object
{
	static CppPrimitive cpp(ref<object<ClassName>> obj)
	{
		return obj->template call_method<MethodName, JniPrimitive()>();
	}

	static ref<object<ClassName>> jni(CppPrimitive value)
	{
		return object<ClassName>::template call_static_method<"valueOf", ref<object<ClassName>>(JniPrimitive)>(value);
	}
};

template <typename T>
concept integer = std::is_integral_v<T> && !std::same_as<T, bool>;

template <typename T, typename I>
concept same_size_integer = integer<T> && integer<I> && sizeof(T) == sizeof(I);

template <object_convertible_to<object<"java/lang/Boolean">> J>
struct convert<ref<J>, bool> : convert_primitive_object<"java/lang/Boolean", bool, jboolean, "booleanValue"> {};

template <object_convertible_to<object<"java/lang/Byte">> J, same_size_integer<std::int8_t> Cpp>
struct convert<ref<J>, Cpp> : convert_primitive_object<"java/lang/Byte", Cpp, jbyte, "byteValue"> {};

template <object_convertible_to<object<"java/lang/Short">> J, same_size_integer<std::int16_t> Cpp>
struct convert<ref<J>, Cpp> : convert_primitive_object<"java/lang/Short", Cpp, jshort, "shortValue"> {};

template <object_convertible_to<object<"java/lang/Integer">> J, same_size_integer<std::int32_t> Cpp>
struct convert<ref<J>, Cpp> : convert_primitive_object<"java/lang/Integer", Cpp, jint, "intValue"> {};

template <object_convertible_to<object<"java/lang/Long">> J, same_size_integer<std::int64_t> Cpp>
struct convert<ref<J>, Cpp> : convert_primitive_object<"java/lang/Long", Cpp, jlong, "longValue"> {};

template <object_convertible_to<object<"java/lang/Float">> J>
struct convert<ref<J>, float> : convert_primitive_object<"java/lang/Float", float, jfloat, "floatValue"> {};

template <object_convertible_to<object<"java/lang/Double">> J>
struct convert<ref<J>, double> : convert_primitive_object<"java/lang/Double", double, jdouble, "doubleValue"> {};

template <primitive J, typename T, typename A>
struct convert<ref<array<J>>, std::vector<T, A>>
{
	static_assert(sizeof(J) == sizeof(T));

	static std::vector<T, A> cpp(ref<array<J>> a)
	{
		const auto size = a->size();
		std::vector<T, A> v(size);
		a->get_region(0, size, reinterpret_cast<J*>(v.data()));
		return v;
	}

	static ref<array<J>> jni(const std::vector<T, A>& v)
	{
		const auto size = static_cast<jsize>(v.size());
		auto a = new_object<array<J>>(size);
		a->set_region(0, size, reinterpret_cast<const J*>(v.data()));
		return a;
	}
};

template <typename A>
struct convert<ref<array<jboolean>>, std::vector<bool, A>>
{
	static std::vector<bool, A> cpp(ref<array<jboolean>> a)
	{
		auto e = a->const_elements<lock::critical>();
		return std::vector<bool, A>(e.begin(), e.end());
	}

	static ref<array<jboolean>> jni(const std::vector<bool, A>& v)
	{
		const auto size = static_cast<jsize>(v.size());
		auto a = new_object<array<jboolean>>(size);
		auto e = a->elements<lock::critical>(size);
		std::copy(v.begin(), v.end(), e.begin());
		return a;
	}
};

template <reference J, typename T, typename A>
struct convert<ref<array<J>>, std::vector<T, A>>
{
	static std::vector<T, A> cpp(ref<array<J>> a)
	{
		const auto size = a->size();
		std::vector<T, A> v(size);

		for (jsize i = 0; i < size; ++i)
			v[i] = convert_cpp<T>(a[i].get());

		return v;
	}

	static ref<array<J>> jni(const std::vector<T, A>& v)
	{
		const auto size = static_cast<jsize>(v.size());
		auto a = new_object<array<J>>(size);

		for (jsize i = 0; i < size; ++i)
			a[i] = convert_jni<ref<J>>(v[i]);

		return a;
	}
};

namespace detail {

using java_util_collection = object<"java/util/Collection", java_lang_iterable>;
using java_util_map = object<"java/util/Map">;

// Non-template JNI loops: one copy in the binary for all element types.
// Callbacks are captureless lambdas (plain function pointers) with a context pointer.

inline void for_each(ref<java_util_collection> c, void (*f)(void*, ref<>), void* ctx)
{
	for (ref<> e : range(c))
		f(ctx, e);
}

inline void for_each(ref<java_util_map> m, void (*f)(void*, ref<>, ref<>), void* ctx)
{
	for (ref<> e : range(m->call_method<"entrySet", ref<object<"java/util/Set", java_lang_iterable>>()>()))
	{
		auto entry = static_pointer_cast<object<"java/util/Map$Entry">>(e);
		f(ctx, entry->call_method<"getKey", ref<>()>(), entry->call_method<"getValue", ref<>()>());
	}
}

inline void add(ref<java_util_collection> c, ref<> e)
{
	c->call_method<"add", jboolean(ref<>)>(std::move(e));
}

inline void put(ref<java_util_map> m, ref<> k, ref<> v)
{
	m->call_method<"put", ref<>(ref<>, ref<>)>(std::move(k), std::move(v));
}

// Initial capacity for HashMap/HashSet to hold size elements without rehashing (default load factor 0.75).

inline jint hash_capacity(std::size_t size)
{
	return static_cast<jint>((std::min<std::size_t>)((size * 4 + 2) / 3, (std::numeric_limits<jint>::max)()));
}

template <typename JC>
ref<JC> new_container(std::size_t size)
{
	if constexpr (std::same_as<JC, object<"java/util/HashMap">> || std::same_as<JC, object<"java/util/HashSet">>)
		return new_object<JC, void(jint)>(hash_capacity(size));
	else
		return new_object<JC, void()>();
}

} // namespace detail

// JC: concrete Java class to create, JI: Java type of the converted reference

template <typename JC, typename JI, typename JE, typename Set>
struct convert_set
{
	static Set cpp(ref<JI> c)
	{
		Set s;

		detail::for_each(static_pointer_cast<detail::java_util_collection>(c), [](void* p, ref<> e)
		{
			auto& s = *static_cast<Set*>(p);
			s.emplace_hint(s.end(), convert_cpp<typename Set::value_type>(static_pointer_cast<JE>(e)));
		}, &s);

		return s;
	}

	static ref<JI> jni(const Set& s)
	{
		auto c = detail::new_container<JC>(s.size());

		for (auto& e : s)
			detail::add(static_pointer_cast<detail::java_util_collection>(c), convert_jni<ref<JE>>(e));

		return static_pointer_cast<JI>(std::move(c));
	}
};

template <typename JC, typename JI, typename JK, typename JV, typename Map>
struct convert_map
{
	static Map cpp(ref<JI> jm)
	{
		Map m;

		detail::for_each(static_pointer_cast<detail::java_util_map>(jm), [](void* p, ref<> k, ref<> v)
		{
			auto& m = *static_cast<Map*>(p);

			m.emplace_hint
			(
				m.end(),
				convert_cpp<typename Map::key_type>(static_pointer_cast<JK>(k)),
				convert_cpp<typename Map::mapped_type>(static_pointer_cast<JV>(v))
			);
		}, &m);

		return m;
	}

	static ref<JI> jni(const Map& m)
	{
		auto jm = detail::new_container<JC>(m.size());

		for (auto& [k, v] : m)
			detail::put(static_pointer_cast<detail::java_util_map>(jm), convert_jni<ref<JK>>(k), convert_jni<ref<JV>>(v));

		return static_pointer_cast<JI>(std::move(jm));
	}
};

using java_util_map = detail::java_util_map;
using java_util_treemap = object<"java/util/TreeMap">;
using java_util_hashmap = object<"java/util/HashMap">;
using java_util_treeset = object<"java/util/TreeSet">;
using java_util_hashset = object<"java/util/HashSet">;

template <typename JK, typename JV, typename K, typename V, typename C, typename A>
struct convert<ref<generic_type<java_util_treemap, JK, JV>>, std::map<K, V, C, A>>
	: convert_map<java_util_treemap, java_util_treemap, JK, JV, std::map<K, V, C, A>> {};

// currently used only manually, not in generated bindings

template <typename JK, typename JV, typename K, typename V, typename C, typename A>
struct convert<ref<generic_type<java_util_map, JK, JV>>, std::map<K, V, C, A>>
	: convert_map<java_util_treemap, java_util_map, JK, JV, std::map<K, V, C, A>> {};

template <typename JK, typename JV, typename K, typename V, typename H, typename KE, typename A>
struct convert<ref<generic_type<java_util_hashmap, JK, JV>>, std::unordered_map<K, V, H, KE, A>>
	: convert_map<java_util_hashmap, java_util_hashmap, JK, JV, std::unordered_map<K, V, H, KE, A>> {};

template <typename JE, typename K, typename C, typename A>
struct convert<ref<generic_type<java_util_treeset, JE>>, std::set<K, C, A>>
	: convert_set<java_util_treeset, java_util_treeset, JE, std::set<K, C, A>> {};

template <typename JE, typename K, typename H, typename KE, typename A>
struct convert<ref<generic_type<java_util_hashset, JE>>, std::unordered_set<K, H, KE, A>>
	: convert_set<java_util_hashset, java_util_hashset, JE, std::unordered_set<K, H, KE, A>> {};

template <typename Jni, typename Struct>
	requires is_struct_v<Struct>
struct convert<Jni, Struct>
{
	using struct_object = object<struct_<Struct>::class_name>;
	using fields = typename struct_<Struct>::fields;

	static ref<struct_object> jni(const Struct& value)
	{
		auto obj = new_object<struct_object, void()>();

		meta::for_each<fields>([&]<typename Field>()
		{
			obj->template set_field<Field::name, typename Field::type>(convert_jni<typename Field::type>(value.*Field::ptr));
		});

		return obj;
	}

	static Struct cpp(ref<struct_object> value)
	{
		Struct obj;

		meta::for_each<fields>([&]<typename Field>()
		{
			obj.*Field::ptr = convert_cpp<std::remove_cvref_t<decltype(obj.*Field::ptr)>>(value->template get_field<Field::name, typename Field::type>());
		});

		return obj;
	}
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_CONVERT_H
