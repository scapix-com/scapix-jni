/*
	scapix/jni/bridge/function.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_BRIDGE_FUNCTION_H
#define SCAPIX_JNI_BRIDGE_FUNCTION_H

#include <concepts>
#include <functional>
#include <utility>
#include <type_traits>
#include <scapix/jni/object.h>
#include <scapix/jni/function.h>
#include <scapix/jni/convert.h>
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

	// Returns this as F, if it is F (without RTTI).

	template <typename F>
	F* target() noexcept
	{
		return type() == &F::type_tag ? static_cast<F*>(this) : nullptr;
	}

private:

	virtual const void* type() const noexcept = 0;

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

	const std::function<R(Args...)>& get() const noexcept { return func; }

private:

	friend class function;

	static constexpr char type_tag = 0;

	const void* type() const noexcept override { return &type_tag; }

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

// Round trip unwraps instead of wrapping again:
// C++ -> Java -> C++ returns (a copy of) the original std::function,
// Java -> C++ -> Java returns the original Java object.

template <fixed_string ClassName, typename JniR, typename ...JniArgs, fixed_string Name, typename R, typename ...Args>
struct convert<ref<function<ClassName, JniR(JniArgs...), Name>>, std::function<R(Args...)>>
{
	using java_function = function<ClassName, JniR(JniArgs...), Name>;

	// std::function target calling Java object.

	struct java_caller
	{
		R operator()(Args&&... args) const
		{
			if constexpr (std::is_void_v<R>)
				return obj->call(convert_jni<JniArgs>(std::forward<Args>(args))...);
			else
				return convert_cpp<R>(obj->call(convert_jni<JniArgs>(std::forward<Args>(args))...));
		}

		global_ref<java_function> obj;
	};

	static std::function<R(Args...)> cpp(ref<java_function> i)
	{
		if (!i)
			return nullptr;

		if (i->template is_instance_of<bridge::function>())
			if (auto f = static_pointer_cast<bridge::function>(i)->get_ptr()->template target<bridge::cpp::function_impl<R(Args...)>>())
				return f->get();

		return java_caller{ global_ref<java_function>(i) };
	}

	static ref<java_function> jni(std::function<R(Args...)> f)
	{
		if (!f)
			return nullptr;

		if (auto c = f.template target<java_caller>())
			return local_ref<java_function>(c->obj);

		return new_object<bridge::function_impl<ClassName, JniR(JniArgs...), Name>>(std::move(f));
	}
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_BRIDGE_FUNCTION_H
