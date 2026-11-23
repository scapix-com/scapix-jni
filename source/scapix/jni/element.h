/*
	scapix/jni/element.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_ELEMENT_H
#define SCAPIX_JNI_ELEMENT_H

namespace scapix::jni {

template <typename T>
struct element_type
{
	using type = T;
};

template <typename T>
using element_type_t = typename element_type<T>::type;

} // namespace scapix::jni

#endif // SCAPIX_JNI_ELEMENT_H
