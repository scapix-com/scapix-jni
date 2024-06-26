/*
	scapix/jni/java_api/init.h

	Copyright (c) 2019-2024 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_JAVA_API_INIT_H
#define SCAPIX_JNI_JAVA_API_INIT_H

#include <scapix/jni/object_base.h>
#include <scapix/jni/ref.h>
#include <scapix/jni/array.h>
#include <scapix/jni/convert.h>

namespace scapix::java_api {

// workaround for conflict between namespaces "sun" and "com::sun"

namespace sun {}
namespace com::sun { namespace sun = ::scapix::java_api::sun; }

} // namespace scapix::java_api

#endif // SCAPIX_JNI_JAVA_API_INIT_H
