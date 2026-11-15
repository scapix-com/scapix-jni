/*
	scapix/jni/detail/api/call.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_DETAIL_API_CALL_H
#define SCAPIX_JNI_DETAIL_API_CALL_H

#include <scapix/jni/detail/api/type.h>
#include <scapix/jni/detail/exception.h>
#include <scapix/jni/ref.h>
#include <scapix/jni/type_traits.h>

namespace scapix::jni::detail::api {

template <typename T, scope Scope>
constexpr jobject call_param(const jni::ref<T, Scope>& v) noexcept
{
	return v.handle();
}

template <primitive T>
constexpr T call_param(T v) noexcept
{
	return v;
}

template <typename T>
struct call;

template <typename R, typename ...Args>
struct call<R(Args...)>
{
	static R method(jobject obj, jmethodID id, Args... args)
	{
		check_exception_nested_on_destroy check;
		return type<R>::call_method(obj, id, api::call_param(args)...);
	}

	static R nonvirtual_method(jobject obj, jclass cls, jmethodID id, Args... args)
	{
		check_exception_nested_on_destroy check;
		return type<R>::call_nonvirtual_method(obj, cls, id, api::call_param(args)...);
	}

	static R static_method(jclass cls, jmethodID id, Args... args)
	{
		check_exception_nested_on_destroy check;
		return type<R>::call_static_method(cls, id, api::call_param(args)...);
	}

	template <typename T>
	static local_ref<T> new_object(jclass cls, jmethodID id, Args... args)
	{
		return check_result_nested<T>(env()->NewObject(cls, id, api::call_param(args)...));
	}
};

} // namespace scapix::jni::detail::api

#endif // SCAPIX_JNI_DETAIL_API_CALL_H
