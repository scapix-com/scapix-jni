/*
	scapix/jni/module.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_MODULE_H
#define SCAPIX_JNI_MODULE_H

#include <exception>
#include <scapix/jni/env.h>
#include <scapix/jni/vm_exception.h>
#include <scapix/jni/detail/init.h>

namespace scapix::jni {

namespace detail {

inline void throw_runtime_exception(const char* message) noexcept
{
	if (jclass cls = env()->FindClass("java/lang/RuntimeException"))
		env()->ThrowNew(cls, message);
}

} // namespace detail

// On error, returns JNI_ERR with pending Java exception.

inline jint on_load(JavaVM* vm, void* reserved, void(*init)() = []{}) noexcept
{
	try
	{
		detail::jvm_ptr = vm;
		get_env();

		detail::init();
		init();

		return JNI_VERSION_1_6;
	}
	catch (const vm_exception& e)
	{
		e.get()->throw_();
	}
	catch (const std::exception& e)
	{
		detail::throw_runtime_exception(e.what());
	}
	catch (...)
	{
		detail::throw_runtime_exception("C++ exception");
	}

	// Android doesn't check exception after calling JNI_OnLoad()
	#ifdef ANDROID
	detail::env()->ExceptionDescribe();
	detail::env()->ExceptionClear();
	#endif

	return JNI_ERR;
}

inline void on_unload(JavaVM *vm, void *reserved) noexcept
{
	detail::jvm_ptr = nullptr;
}

} // namespace scapix::jni

#endif // SCAPIX_JNI_MODULE_H
