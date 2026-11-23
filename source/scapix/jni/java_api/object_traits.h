/*
	scapix/jni/java_api/object_traits.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_JAVA_API_OBJECT_TRAITS_H
#define SCAPIX_JNI_JAVA_API_OBJECT_TRAITS_H

#include <scapix/jni/object_traits.h>

// Generated code specializes object_traits in its own namespace,
// where names of generated classes do not need to be qualified.

namespace scapix::java_api {

template <typename T>
struct object_traits;

} // namespace scapix::java_api

namespace scapix::jni {

template <typename T>
	requires requires { java_api::object_traits<T>::class_name; }
struct object_traits<T> : java_api::object_traits<T>
{
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_JAVA_API_OBJECT_TRAITS_H
