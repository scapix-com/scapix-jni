/*
	scapix/jni/fwd/native_method.h

	Copyright (c) 2019-2024 Boris Rasin (boris@scapix.com)
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

// deduce_signature

namespace detail {

template <typename T>
struct deduce_signature_impl;

template <class_type Class, return_type R, type... Args>
struct deduce_signature_impl<R(Class::*)(Args...)>
{
	using type = R(Args...);
};

template <return_type R, reference T, type... Args>
struct deduce_signature_impl<R(ref<T>, Args...)>
{
	using type = R(Args...);
};

} // namespace detail

template <typename CppType>
using deduce_signature = typename detail::deduce_signature_impl<remove_function_qualifiers_t<std::remove_pointer_t<CppType>>>::type;

// native_method_info

template <method JniType, typename CppType, std::size_t N, bool AndroidCriticalNative = false>
struct native_method_info
{
	using jni_type = JniType;
	using cpp_type = CppType;
	constexpr static bool android_critical_native = AndroidCriticalNative;

	constexpr native_method_info(fixed_string<char, N> name, cpp_type method) : name(name), method(method) {}

	fixed_string<char, N> name;
	cpp_type method;
};

template <fixed_string ClassName, native_method_info ...Methods>
class native_methods;

// native_method_helper

template <function_type Signature>
struct native_method_helper
{
	template <class_type Class>
	using member_ptr = Signature Class::*;
};

template <typename R, typename... Args>
struct native_method_helper<R(Args...)>
{
	template <reference T>
	using ptr = R(*)(ref<T>, Args...);

	template <class_type Class>
	using member_ptr = R(Class::*)(Args...);
};

template <typename R, typename... Args>
struct native_method_helper<R(Args...) noexcept>
{
	template <reference T>
	using ptr = R(*)(ref<T>, Args...) noexcept;

	template <class_type Class>
	using member_ptr = R(Class::*)(Args...) noexcept;
};

// native_method

// For an instance method, the first parameter must be ref<object<"com/example/MyClass">> (or compatible).
// For a static method, the first parameter must be ref<class_> (or compatible).
// If all parameters are JNI types, JNI signature can be deduced.
// If you omit ref<class_> first parameter for a static method or use any C++ type parameters, you must specify JNI signature.
// Additionally, you can specify C++ signature to select overload.

// Both types specified - use CppType to select overload.

template <method JniType, function_type CppType, reference T, std::size_t N>
constexpr auto native_method(const char(&name)[N], typename native_method_helper<CppType>::template ptr<T> ptr)
{
	return native_method_info<JniType, decltype(ptr), N>(name, ptr);
}

template <method JniType, function_type CppType, class_type Class, std::size_t N>
constexpr auto native_method(const char(&name)[N], typename native_method_helper<CppType>::template member_ptr<Class> ptr)
{
	return native_method_info<JniType, decltype(ptr), N>(name, ptr);
}

// static without ref<class_> parameter

template <method JniType, function_type CppType, std::size_t N>
constexpr auto native_method(const char(&name)[N], std::add_pointer_t<CppType> ptr)
{
	return native_method_info<JniType, decltype(ptr), N>(name, ptr);
}

// Only JniType specified - use JniType to select overload.

template <method JniType, reference T, std::size_t N>
constexpr auto native_method(const char(&name)[N], typename native_method_helper<JniType>::template ptr<T> ptr)
{
	return native_method_info<JniType, decltype(ptr), N>(name, ptr);
}

template <method JniType, class_type Class, std::size_t N>
constexpr auto native_method(const char(&name)[N], typename native_method_helper<JniType>::template member_ptr<Class> ptr)
{
	return native_method_info<JniType, decltype(ptr), N>(name, ptr);
}

// static without ref<class_> parameter

// workaround for msvc 19.51 bug
// https://developercommunity.microsoft.com/t/MSVC-C-overload-resolution-reports-C26/11143435

//template <method JniType, std::size_t N>
template <typename JniType, std::size_t N>
	requires method<JniType>
constexpr auto native_method(const char(&name)[N], std::add_pointer_t<JniType> ptr)
{
	return native_method_info<JniType, decltype(ptr), N>(name, ptr);
}

template <method JniType, std::size_t N>
constexpr auto native_method(const char(&name)[N], auto ptr)
{
	return native_method_info<JniType, decltype(ptr), N>(name, ptr);
}

// No types specified - only works if signature can be deduced and only for single overload.

template <std::size_t N>
constexpr auto native_method(const char(&name)[N], auto ptr)
{
	return native_method_info<deduce_signature<decltype(ptr)>, decltype(ptr), N>(name, ptr);
}

// Android method marked @CriticalNative
// https://developer.android.com/reference/dalvik/annotation/optimization/CriticalNative

template <android_critical_native_type Type, std::size_t N>
constexpr auto android_critical_native(const char(&name)[N], Type* ptr)
{
	return native_method_info<Type, decltype(ptr), N, true>(name, ptr);
}

} // namespace scapix::jni

#endif // SCAPIX_JNI_FWD_NATIVE_METHOD_H
