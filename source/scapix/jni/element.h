/*
	scapix/jni/element.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_ELEMENT_H
#define SCAPIX_JNI_ELEMENT_H

#include <scapix/jni/fwd/element.h>
#include <scapix/jni/fwd/array.h>

namespace scapix::jni {

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

} // namespace scapix::jni

#endif // SCAPIX_JNI_ELEMENT_H
