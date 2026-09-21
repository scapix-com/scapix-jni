/*
	scapix/jni/bridge/function.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_BRIDGE_FUNCTION_H
#define SCAPIX_JNI_BRIDGE_FUNCTION_H

#include <functional>
#include <utility>
#include <scapix/jni/object.h>
#include <scapix/jni/function.h>
#include <scapix/jni/fwd/native_method.h>

namespace scapix::jni {
namespace bridge {
namespace cpp {

class function
{
public:

	virtual ~function() = default;

	void finalize()
	{
		delete this;
	}

};

template <typename T>
class function_impl;

template <typename R, typename ...Args>
class function_impl<R(Args...)> : public function
{
public:

	function_impl(std::function<R(Args...)>&& f) : func(std::move(f)) {}

	R call(Args... args)
	{
		return func(std::forward<Args>(args)...);
	}

private:

	std::function<R(Args...)> func;

};

} // namespace cpp

class function : public object<"com/scapix/Function">
{
public:

	using native_methods = jni::native_methods
	<
		class_name,
		native_method("finalize", &cpp::function::finalize)
	>;

	cpp::function* get_ptr() const
	{
		return reinterpret_cast<cpp::function*>(get_field<"ptr", jlong>());
	}

};

template <fixed_string InterfaceClassName, typename Type, fixed_string Name>
class function_impl;

template <fixed_string InterfaceClassName, typename JniR, typename ...JniArgs, fixed_string Name>
class function_impl<InterfaceClassName, JniR(JniArgs...), Name> : public object<InterfaceClassName + "Impl",
	function,
	jni::function<InterfaceClassName, JniR(JniArgs...), Name>>
{
public:

	template <typename R, typename ...Args>
	static local_ref<function_impl> new_object(std::function<R(Args...)>&& func)
	{
		cpp::function* ptr = new cpp::function_impl<R(Args...)>(std::move(func));
		return jni::new_object<function_impl, void(jlong)>(reinterpret_cast<jlong>(ptr));
	}

};

} // namespace bridge

template <std::derived_from<bridge::cpp::function> T, typename J>
T& convert_this(ref<J> x)
{
	return *static_cast<T*>(static_pointer_cast<bridge::function>(x)->get_ptr());
}

template <fixed_string ClassName, typename JniR, typename ...JniArgs, fixed_string Name, typename R, typename ...Args>
struct convert<ref<function<ClassName, JniR(JniArgs...), Name>>, std::function<R(Args...)>>
{
	static std::function<R(Args...)> cpp(ref<function<ClassName, JniR(JniArgs...), Name>> i)
	{
		if (!i)
			return nullptr;

		return std::function<R(Args...)>([i = global_ref<function<ClassName, JniR(JniArgs...), Name>>(i)](Args&&... args)
		{
			if constexpr (std::is_void_v<R>)
				return i->call(convert_jni<JniArgs>(std::forward<Args>(args))...);
			else
				return convert_cpp<R>(i->call(convert_jni<JniArgs>(std::forward<Args>(args))...));
		});
	}

	static ref<function<ClassName, JniR(JniArgs...), Name>> jni(std::function<R(Args...)> f)
	{
		if (!f)
			return nullptr;

		return new_object<bridge::function_impl<ClassName, JniR(JniArgs...), Name>>(std::move(f));
	}
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_BRIDGE_FUNCTION_H
