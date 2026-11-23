/*
	scapix/jni/signature.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_SIGNATURE_H
#define SCAPIX_JNI_SIGNATURE_H

#include <type_traits>
#include <scapix/core/fixed_string.h>
#include <scapix/jni/env.h>
#include <scapix/jni/type_traits.h>
#include <scapix/jni/fwd/signature.h>

namespace scapix::jni {

// signature
//
// JNI type signature (field and method descriptor) of Java type: erased type,
// not Java generic signature (like "Ljava/util/List<Ljava/lang/String;>;").
//
// Defined for Java types only (primitives, object types, arrays), not for JNI value types like ref<T>.
// Method type R(Args...) uses JNI value types, mapped to Java types by to_java_type.
//
// Reference type: derived from its class name.
// Array class names are already descriptors (like "[Ljava/lang/String;"), others are wrapped in "L...;".

template <typename T>
struct signature
{
	static_assert(!std::is_same_v<T, T>, "scapix::jni::signature: not a Java type (for ref<T>, use T)");
	static constexpr fixed_string value = "";
};

template <object_type T>
struct signature<T>
{
	static constexpr auto value = "L" + class_name_v<T> + ";";
};

template <array_type T>
struct signature<T>
{
	static constexpr auto value = class_name_v<T>;
};

template<> struct signature<void>     { static constexpr fixed_string value = "V"; };
template<> struct signature<jboolean> { static constexpr fixed_string value = "Z"; };
template<> struct signature<jbyte>    { static constexpr fixed_string value = "B"; };
template<> struct signature<jchar>    { static constexpr fixed_string value = "C"; };
template<> struct signature<jshort>   { static constexpr fixed_string value = "S"; };
template<> struct signature<jint>     { static constexpr fixed_string value = "I"; };
template<> struct signature<jlong>    { static constexpr fixed_string value = "J"; };
template<> struct signature<jfloat>   { static constexpr fixed_string value = "F"; };
template<> struct signature<jdouble>  { static constexpr fixed_string value = "D"; };

template <typename R, typename ...Args>
struct signature<R(Args...)>
{
	static constexpr auto value = ("(" + ... + signature_v<to_java_type_t<Args>>) + (")" + signature_v<to_java_type_t<R>>);
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_SIGNATURE_H
