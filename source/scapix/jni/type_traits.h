/*
	scapix/jni/type_traits.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_TYPE_TRAITS_H
#define SCAPIX_JNI_TYPE_TRAITS_H

#include <bit>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <scapix/core/fixed_string.h>
#include <scapix/core/meta/any_of.h>
#include <scapix/jni/env.h>
#include <scapix/jni/object_traits.h>
#include <scapix/jni/element.h>
#include <scapix/jni/generics.h>
#include <scapix/jni/fwd/ref.h>

namespace scapix::jni {

// primitive

template <typename T>
concept primitive =
	std::same_as<T, jboolean> ||
	std::same_as<T, jbyte> ||
	std::same_as<T, jchar> ||
	std::same_as<T, jshort> ||
	std::same_as<T, jint> ||
	std::same_as<T, jlong> ||
	std::same_as<T, jfloat> ||
	std::same_as<T, jdouble>;

template <typename T>
concept primitive_return_type = primitive<T> || std::is_void_v<T>;

// java_integral_type

template <std::integral T>
using java_integral_type = std::tuple_element_t<std::bit_width(sizeof(T)) - 1, std::tuple<jbyte, jshort, jint, jlong>>;

// java_type

template <typename T>
concept java_type = object_type<T> || primitive<T>;

// array_type

template <typename T>
concept array_type = object_type<T> && class_name_v<T>[0] == '[';

template <typename T>
concept object_array = array_type<T> && (class_name_v<T>[1] == 'L' || class_name_v<T>[1] == '[');

// handle_type

namespace detail {

// Specializations instead of class name comparisons: much faster to compile (especially with MSVC).

template <fixed_string ClassName> struct handle_type_of { using type = void; };
template <> struct handle_type_of<"java/lang/Class"> { using type = jclass; };
template <> struct handle_type_of<"java/lang/String"> { using type = jstring; };
template <> struct handle_type_of<"java/lang/Throwable"> { using type = jthrowable; };
template <> struct handle_type_of<"[Z"> { using type = jbooleanArray; };
template <> struct handle_type_of<"[B"> { using type = jbyteArray; };
template <> struct handle_type_of<"[C"> { using type = jcharArray; };
template <> struct handle_type_of<"[S"> { using type = jshortArray; };
template <> struct handle_type_of<"[I"> { using type = jintArray; };
template <> struct handle_type_of<"[J"> { using type = jlongArray; };
template <> struct handle_type_of<"[F"> { using type = jfloatArray; };
template <> struct handle_type_of<"[D"> { using type = jdoubleArray; };

template <object_type T>
constexpr auto handle_type()
{
	using type = typename handle_type_of<class_name_v<T>>::type;
	using bases = base_classes_t<T>;

	if constexpr (!std::is_void_v<type>) return type();
	else if constexpr (class_name_v<T>[0] == '[') return jobjectArray();
	else if constexpr (std::tuple_size_v<bases> != 0) return handle_type<std::tuple_element_t<0, bases>>();
	else return jobject();
}

} // namespace detail

template <object_type T>
using handle_type_t = decltype(detail::handle_type<T>());

// remove_erased
//
// erased<T, Erasure>               ->  T
// ref<erased<T, Erasure>, Scope>   ->  ref<T, Scope>

template <typename T>
struct remove_erased
{
	using type = T;
};

template <typename T>
using remove_erased_t = typename remove_erased<T>::type;

template <typename T, typename Erasure>
struct remove_erased<erased<T, Erasure>>
{
	using type = T;
};

template <typename T, scope Scope>
struct remove_erased<ref<T, Scope>>
{
	using type = ref<remove_erased_t<T>, Scope>;
};

// object_convertible_to

namespace detail {

template <object_type From, object_type To>
struct is_convertible
{
	template <typename T>
	struct is_base_convertible : is_convertible<T, To> {};

	static constexpr bool value =
		class_name_v<From> == class_name_v<To> ||
		meta::any_of_v<base_classes_t<From>, is_base_convertible>
	;
};

template <object_array From, object_array To>
struct is_convertible<From, To>
{
	static constexpr bool value = is_convertible<typename From::value_type::element_type, typename To::value_type::element_type>::value;
};

} // namespace detail

template <typename From, typename To>
concept object_convertible_to = object_type<From> && object_type<To> && detail::is_convertible<From, To>::value;

// same_class_as
// Same Java class, regardless of C++ type (for example, object<"java/util/HashMap"> and generated java::util::HashMap).

template <typename T1, typename T2>
concept same_class_as = object_type<T1> && object_type<T2> && class_name_v<T1> == class_name_v<T2>;

// param_type

template <typename T>
concept param_type = ref_type<T> || primitive<T>;

// return_type

template <typename T>
concept return_type = param_type<T> || std::is_void_v<T>;

// to_java_type
//
// Java type of JNI value type (method parameter, return value or field):
// ref<T> -> T, primitive -> itself.

template <typename T>
struct to_java_type
{
	using type = T;
};

template <typename T, scope Scope>
struct to_java_type<ref<T, Scope>>
{
	using type = T;
};

template <typename T>
using to_java_type_t = typename to_java_type<T>::type;

// method_type, init_method_type, android_critical_native_method_type

namespace detail {

template <typename F>
constexpr bool is_method_type = false;

template <typename R, typename ...Args>
constexpr bool is_method_type<R(Args...)> = return_type<R> && (param_type<Args> && ...);

template <typename R, typename ...Args>
constexpr bool is_method_type<R(Args...) noexcept> = is_method_type<R(Args...)>;

template <typename F>
constexpr bool is_init_method_type = false;

template <typename ...Args>
constexpr bool is_init_method_type<void(Args...)> = (param_type<Args> && ...);

template <typename ...Args>
constexpr bool is_init_method_type<void(Args...) noexcept> = is_init_method_type<void(Args...)>;

template <typename F>
constexpr bool is_android_critical_native_method_type = false;

template <typename R, typename ...Args>
constexpr bool is_android_critical_native_method_type<R(Args...)> = primitive_return_type<R> && (primitive<Args> && ...);

template <typename R, typename ...Args>
constexpr bool is_android_critical_native_method_type<R(Args...) noexcept> = is_android_critical_native_method_type<R(Args...)>;

} // namespace detail

template <typename F>
concept method_type = detail::is_method_type<F>;

template <typename F>
concept init_method_type = detail::is_init_method_type<F>;

template <typename F>
concept android_critical_native_method_type = detail::is_android_critical_native_method_type<F>;

} // namespace scapix::jni

#endif // SCAPIX_JNI_TYPE_TRAITS_H
