/*
	scapix/jni/byte_buffer.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_BYTE_BUFFER_H
#define SCAPIX_JNI_BYTE_BUFFER_H

#include <scapix/jni/object.h>
#include <scapix/jni/buffer.h>

namespace scapix::jni {

class byte_buffer : public object<"java/nio/ByteBuffer", buffer>
{
public:

	static local_ref<byte_buffer> new_direct(void* address, jlong capacity)
	{
		return detail::check_result<byte_buffer>(detail::env()->NewDirectByteBuffer(address, capacity));
	}

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_BYTE_BUFFER_H
