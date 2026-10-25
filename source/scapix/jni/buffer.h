/*
	scapix/jni/buffer.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_BUFFER_H
#define SCAPIX_JNI_BUFFER_H

#include <scapix/jni/object.h>

namespace scapix::jni {

class buffer : public object<"java/nio/Buffer">
{
public:

	void* direct_address() const
	{
		return detail::env()->GetDirectBufferAddress(handle());
	}

	jlong direct_capacity() const
	{
		return detail::env()->GetDirectBufferCapacity(handle());
	}

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_BUFFER_H
