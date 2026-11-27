/*
	scapix/jni/lock.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_LOCK_H
#define SCAPIX_JNI_LOCK_H

#include <scapix/jni/env.h>

namespace scapix::jni {

enum class lock
{
	noncritical,
	critical,
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_LOCK_H
