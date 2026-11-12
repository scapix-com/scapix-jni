/*
	scapix/jni/error.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_ERROR_H
#define SCAPIX_JNI_ERROR_H

#include <stdexcept>

namespace scapix::jni {

// JNI function reported an error without throwing a Java exception.

class error : public std::runtime_error
{
public:

	using std::runtime_error::runtime_error;

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_ERROR_H
