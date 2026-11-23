/*
	scapix/jni/generics.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_GENERICS_H
#define SCAPIX_JNI_GENERICS_H

#include <scapix/jni/element.h>
#include <scapix/jni/fwd/object.h>

namespace scapix::jni {

// parameterized
//
// Parameterized type (like Java Map<String, Integer>) when not using generated headers.
//
// With generated headers: ref<java::util::Set<java::lang::String>>
// Without generated headers: ref<parameterized<java_util_set, object<"java/lang/String">>>
//
// Object is a raw (non-generic) reference type, which provides class name and base classes.

template <typename Object, typename ...Args>
class parameterized;

template <typename Object, typename ...Args>
struct element_type<parameterized<Object, Args...>>
{
	using type = Object;
};

// erased
//
// Type T with JNI signature of Erasure.
//
// Java erases type variables to their first bound (java.lang.Object, if not bounded):
// the signature of "E get(int)" in List<E> is "(I)Ljava/lang/Object;", for any E.
//
// list->call_method<"get", ref<erased<java::lang::String>>(jint)>(0);

template <typename T, typename Erasure = object<>>
class erased;

template <typename T, typename Erasure>
struct element_type<erased<T, Erasure>>
{
	using type = element_type_t<T>;
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_GENERICS_H
