/*
	scapix/jni/object_traits.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_OBJECT_TRAITS_H
#define SCAPIX_JNI_OBJECT_TRAITS_H

#include <scapix/jni/element.h>

namespace scapix::java_api {

// Generated code specializes java_api::object_traits,
// where names of generated classes do not need to be qualified.
// Otherwise, uses traits from (complete) object type.

template <typename T>
struct object_traits {};

template <typename T>
	requires requires { T::class_name; typename T::base_classes; }
struct object_traits<T>
{
	static constexpr auto class_name = T::class_name;
	using base_classes = typename T::base_classes;
};

} // namespace scapix::java_api

namespace scapix::jni {

// object_traits
// Specialized to provide traits for incomplete object type.
// Otherwise, uses java_api::object_traits.

template <typename T>
struct object_traits : java_api::object_traits<T> {};

// object_type

template <typename T>
concept object_type = requires { object_traits<element_type_t<T>>::class_name; };

template <object_type T>
constexpr auto class_name_v = object_traits<element_type_t<T>>::class_name;

template <object_type T>
using base_classes_t = typename object_traits<element_type_t<T>>::base_classes;

} // namespace scapix::jni

#endif // SCAPIX_JNI_OBJECT_TRAITS_H
