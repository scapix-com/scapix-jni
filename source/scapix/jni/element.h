/*
	scapix/jni/element.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_ELEMENT_H
#define SCAPIX_JNI_ELEMENT_H

#include <scapix/jni/fwd/element.h>
#include <scapix/jni/fwd/object.h>
#include <scapix/jni/fwd/array.h>

namespace scapix::jni {

template <typename T, typename ...Params>
struct generic_type;

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

//template <typename T>
//struct extends;

//template <typename T>
//struct super;

template <typename T>
struct element_type
{
	using type = T;
};

template <typename T>
struct element_type<T[]>
{
	using type = array<T>;
};

template <typename T, typename ...Params>
struct element_type<generic_type<T, Params...>>
{
	using type = element_type_t<T>;
};

template <typename T, typename Erasure>
struct element_type<erased<T, Erasure>>
{
	using type = element_type_t<T>;
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_ELEMENT_H
