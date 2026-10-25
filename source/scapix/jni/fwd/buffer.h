/*
	scapix/jni/fwd/buffer.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_FWD_BUFFER_H
#define SCAPIX_JNI_FWD_BUFFER_H

#include <tuple>
#include <scapix/core/fixed_string.h>
#include <scapix/jni/object_traits.h>
#include <scapix/jni/fwd/object.h>

namespace scapix::jni {

class buffer;

template<>
struct object_traits<buffer>
{
	static constexpr auto class_name = "java/nio/Buffer";
	using base_classes = std::tuple<object<>>;
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_FWD_BUFFER_H
