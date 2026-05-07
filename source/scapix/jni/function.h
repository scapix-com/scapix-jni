/*
	scapix/jni/function.h

	Copyright (c) 2019-2024 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_FUNCTION_H
#define SCAPIX_JNI_FUNCTION_H

#include <scapix/jni/object.h>

namespace scapix::jni {

// represents java @FunctionalInterface

template <fixed_string ClassName, typename Type, fixed_string Name = "call">
class function;

template <fixed_string ClassName, typename R, typename ...Args, fixed_string Name>
class function<ClassName, R(Args...), Name> : public object<ClassName>
{
public:

	R call(Args... args) const
	{
		return this->template call_method<Name, R(Args...)>(args...);
	}

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_FUNCTION_H
