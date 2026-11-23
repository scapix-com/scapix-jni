/*
	scapix/jni/fwd/native_method.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_FWD_NATIVE_METHOD_H
#define SCAPIX_JNI_FWD_NATIVE_METHOD_H

#include <type_traits>
#include <scapix/core/type_traits.h>
#include <scapix/core/fixed_string.h>
#include <scapix/core/remove_function_qualifiers.h>
#include <scapix/jni/type_traits.h>
#include <scapix/jni/fwd/class.h>

namespace scapix::jni {

namespace detail {

template <typename T>
struct deduce_signature;

template <class_type Class, return_type R, param_type... Args>
struct deduce_signature<R(Class::*)(Args...)>
{
	using type = R(Args...);
};

template <return_type R, object_type T, param_type... Args>
struct deduce_signature<R(ref<T>, Args...)>
{
	using type = R(Args...);
};

template <typename Signature>
struct prepend_ref_impl {};

template <typename R, typename... Args>
struct prepend_ref_impl<R(Args...)>
{
	template <object_type T>
	using type = R(ref<T>, Args...);
};

template <typename R, typename... Args>
struct prepend_ref_impl<R(Args...) noexcept>
{
	template <object_type T>
	using type = R(ref<T>, Args...) noexcept;
};

template <typename Signature, typename T>
using prepend_ref = typename prepend_ref_impl<Signature>::template type<T>;

} // namespace detail

template <typename CppType>
using deduce_signature = typename detail::deduce_signature<remove_function_qualifiers_t<std::remove_pointer_t<CppType>>>::type;

// native_method_info

template <method_type JniType, typename CppType, std::size_t N, bool AndroidCriticalNative = false>
struct native_method_info
{
	using jni_type = JniType;
	using cpp_type = CppType;
	constexpr static bool android_critical_native = AndroidCriticalNative;

	fixed_string<char, N> name;
	cpp_type method;
};

template <fixed_string ClassName, native_method_info ...Methods>
class native_methods;

namespace detail {

template <method_type JniType, bool AndroidCriticalNative = false>
constexpr auto native_method(compile_time_string auto&& name, auto ptr)
{
	return native_method_info<JniType, decltype(ptr), string_size<decltype(name)>(), AndroidCriticalNative>{name, ptr};
}

} // namespace detail

// native_method

// For an instance method, the first parameter must be ref<object<"com/example/MyClass">> (or compatible).
// For a static method, the first parameter must be ref<class_> (or compatible).
// If all parameters are JNI types, JNI signature can be deduced.
// If you omit ref<class_> first parameter for a static method or use any C++ type parameters, you must specify JNI signature.
// Additionally, you can specify C++ signature to select overload.

// Both types specified - use CppType to select overload.

template <method_type JniType, function_type CppType, object_type T>
constexpr auto native_method(compile_time_string auto&& name, detail::prepend_ref<CppType, T>* ptr)
{
	return detail::native_method<JniType>(name, ptr);
}

template <method_type JniType, function_type CppType, class_type Class>
constexpr auto native_method(compile_time_string auto&& name, std::type_identity_t<CppType> Class::* ptr)
{
	return detail::native_method<JniType>(name, ptr);
}

// Only JniType specified - use JniType to select overload.

template <method_type JniType, object_type T>
constexpr auto native_method(compile_time_string auto&& name, detail::prepend_ref<JniType, T>* ptr)
{
	return detail::native_method<JniType>(name, ptr);
}

template <method_type JniType, class_type Class>
constexpr auto native_method(compile_time_string auto&& name, std::type_identity_t<JniType> Class::* ptr)
{
	return detail::native_method<JniType>(name, ptr);
}

// static without ref<class_> parameter
// Accepts any single function or selects overload based on CppType (which defaults to JniType).

template <method_type JniType, function_type CppType = JniType, typename Ptr = std::add_pointer_t<CppType>>
constexpr auto native_method(compile_time_string auto&& name, Ptr ptr)
{
	return detail::native_method<JniType>(name, ptr);
}

// No types specified - only works if signature can be deduced and only for single overload.

constexpr auto native_method(compile_time_string auto&& name, auto ptr)
{
	return detail::native_method<deduce_signature<decltype(ptr)>>(name, ptr);
}

// Android method marked @CriticalNative
// https://developer.android.com/reference/dalvik/annotation/optimization/CriticalNative

template <android_critical_native_method_type Type>
constexpr auto android_critical_native_method(compile_time_string auto&& name, Type* ptr)
{
	return detail::native_method<Type, true>(name, ptr);
}

} // namespace scapix::jni

#endif // SCAPIX_JNI_FWD_NATIVE_METHOD_H
