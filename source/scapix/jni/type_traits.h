/*
	scapix/jni/type_traits.h

	Copyright (c) 2019-2024 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_TYPE_TRAITS_H
#define SCAPIX_JNI_TYPE_TRAITS_H

#include <type_traits>
#include <scapix/core/fixed_string.h>
#include <scapix/core/meta/any_of.h>
#include <scapix/core/meta/for_each.h>
#include <scapix/core/meta/iota.h>
#include <scapix/jni/env.h>
#include <scapix/jni/object_traits.h>
#include <scapix/jni/fwd/element.h>
#include <scapix/jni/fwd/array.h>
#include <scapix/jni/fwd/object.h>
#include <scapix/jni/fwd/object_base.h>
#include <scapix/jni/fwd/ref.h>

namespace scapix::jni {

// reference

template <typename T>
concept reference = requires
{
	requires std::is_class_v<element_type_t<T>>;
	class_name_v<T>;
	typename base_classes_t<T>;
};

// primitive

template <typename T>
concept integral =
	std::same_as<T, jbyte> ||
	std::same_as<T, jshort> ||
	std::same_as<T, jint> ||
	std::same_as<T, jlong> ||
	std::same_as<T, jchar>;

template <typename T>
concept floating_point =
	std::same_as<T, jfloat> ||
	std::same_as<T, jdouble>;

template <typename T>
concept numeric = integral<T> || floating_point<T>;

template <typename T>
concept primitive = numeric<T> || std::same_as<T, jboolean>;

template <typename T>
concept primitive_or_void = primitive<T> || std::same_as<T, void>;

// array_element

template <typename T>
concept array_element = reference<T> || primitive<T>;

// array

template <typename T>
concept is_array = reference<T> && class_name_v<T>[0] == '[';

template <typename T>
concept object_array = reference<T> && class_name_v<T>[0] == '[' && (class_name_v<T>[1] == 'L' || class_name_v<T>[1] == '[');

template <typename T>
concept primitive_array = reference<T> && class_name_v<T>[0] == '[' && class_name_v<T>[1] != 'L' && class_name_v<T>[1] != '[';

template <typename T>
concept numeric_array = reference<T> && class_name_v<T>[0] == '[' && class_name_v<T>[1] != 'L' && class_name_v<T>[1] != '[' && class_name_v<T>[1] != 'Z';

// handle_type

namespace detail {

template <fixed_string ClassName, typename HandleType>
struct handle_type_info
{
	static constexpr auto class_name = ClassName;
	using handle_type = HandleType;
};

using handle_types = std::tuple
<
	handle_type_info<"java/lang/Object", jobject>, // must be first
	handle_type_info<"java/lang/Class", jclass>,
	handle_type_info<"java/lang/String", jstring>,
	handle_type_info<"java/lang/Throwable", jthrowable>,
	handle_type_info<"[Z", jbooleanArray>,
	handle_type_info<"[B", jbyteArray>,
	handle_type_info<"[C", jcharArray>,
	handle_type_info<"[S", jshortArray>,
	handle_type_info<"[I", jintArray>,
	handle_type_info<"[J", jlongArray>,
	handle_type_info<"[F", jfloatArray>,
	handle_type_info<"[D", jdoubleArray>,
	handle_type_info<"[", jobjectArray> // uses "begins_with" compare, must be last
>;

template <reference T>
constexpr std::size_t handle_type()
{
	auto class_name = class_name_v<T>;
	using bases = base_classes_t<T>;

	std::size_t result = 0;

	meta::for_each<meta::iota_c<std::tuple_size_v<handle_types>>>([&]<typename Index>()
	{
		if (std::tuple_element_t<Index::value, handle_types>::class_name == class_name)
			result = Index::value;
	});

	if (!result)
	{
		if (class_name[0] == '[')
		{
			result = std::tuple_size_v<handle_types> - 1;
		}
		else if constexpr (std::tuple_size_v<bases> != 0)
		{
			result = handle_type<std::tuple_element_t<0, bases>>();
		}
	}

	return result;
}

} // namespace detail

template <reference T>
using handle_type_t = typename std::tuple_element_t<detail::handle_type<T>(), detail::handle_types>::handle_type;

// is_convertible_object

template <reference From, reference To>
struct is_convertible_object;

template <reference From, reference To>
constexpr bool is_convertible_object_v = is_convertible_object<From, To>::value;

template <typename From, typename To>
concept object_convertible_to = is_convertible_object_v<From, To>;

template <reference From, reference To>
struct is_convertible_object
{
	template <typename T>
	struct is_convertible : is_convertible_object<T, To> {};

	static constexpr bool value =
		class_name_v<From> == class_name_v<To> ||
		meta::any_of_v<base_classes_t<From>, is_convertible>
	;
};

template <object_array From, object_array To>
struct is_convertible_object<From, To>
{
	static constexpr bool value = is_convertible_object_v<typename From::value_type::element_type, typename To::value_type::element_type>;
};


// compatible_object

template <typename T1, typename T2>
concept compatible_object = reference<T1> && reference<T2> && class_name_v<T1> == class_name_v<T2>;

// type

template <typename T>
concept type = is_ref<T> || primitive<T>;

// return_type

template <typename T>
concept return_type = type<T> || std::is_void_v<T>;

// method

template <typename F>
concept method = requires (F* f)
{
// clang 22 bug
#ifdef __clang__
	[] <typename R, typename ...Args> (R(*)(Args...)) {}(f);
#else
	[] <return_type R, type ...Args> (R(*)(Args...)) {}(f);
#endif
};

// init_method

template <typename F>
concept init_method = requires (F* f)
{
// clang 22 bug
#ifdef __clang__
	[] <typename ...Args> (void(*)(Args...)) {}(f);
#else
	[] <type ...Args> (void(*)(Args...)) {}(f);
#endif
};

// android_critical_native_type

template <typename F>
concept android_critical_native_type = requires (F* f)
{
	[] <primitive_or_void R, primitive ...Args> (R(*)(Args...)) {}(f);
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_TYPE_TRAITS_H
