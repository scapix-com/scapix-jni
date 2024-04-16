/*
	scapix/jni/native_method.h

	Copyright (c) 2019-2024 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_NATIVE_METHOD_H
#define SCAPIX_JNI_NATIVE_METHOD_H

#include <scapix/jni/fwd/native_method.h>
#include <scapix/core/tuple.h>
#include <scapix/core/remove_function_qualifiers.h>
#include <scapix/core/fixed_string.h>
#include <scapix/jni/convert.h>
#include <scapix/jni/signature.h>
#include <scapix/jni/vm_exception.h>
#include <scapix/jni/com/scapix/native_exception.h>
#include <scapix/jni/class.h>

namespace scapix::jni {

template <typename Jni, typename Cpp>
struct param;

template <primitive Jni, typename Cpp>
struct param<Jni, Cpp>
{
	using Cpv = std::remove_reference_t<Cpp>;

	static Jni jni(Cpp v) { return convert_jni<Jni>(v); }
	static Cpv cpp(Jni v) { return convert_cpp<Cpp>(v); }
};

template <typename T, typename Cpp>
struct param<ref<T>, Cpp>
{
	static jobject jni(Cpp&& v)
	{
		ref<T> res(std::move(v));

		assert(res.get_scope() == scope::local || res.get_scope() == scope::generic);

//      if (res.get_scope() == scope::global || res.get_scope() == scope::weak)
//          return local_ref<>(res).release();

		return res.release();
	}

	static decltype(auto) cpp(jobject v)
	{
		return ref<T>(v);
	}
};

template <typename Jni>
struct param_t;

template <primitive_or_void Jni>
struct param_t<Jni>
{
	using type = Jni;
};

template <typename T>
struct param_t<ref<T>>
{
	using type = jobject;
};

template <typename T>
using param_type = typename param_t<T>::type;

template <function_pointer Func>
struct jni_native_method
{
	const char* name;
	const char* signature;
	Func fnPtr;

private:

	static void compile_check()
	{
		static_assert(sizeof(JNINativeMethod) == sizeof(jni_native_method), "jni_native_method should be ABI compatible with JNINativeMethod");
	}

};

// for apple clang 15.0.0.15000100
template <typename Func>
jni_native_method(const char*, const char*, Func) -> jni_native_method<Func>;

// native_method_impl

template <typename JniR, typename R, typename F>
param_type<JniR> invoke(JNIEnv* env, F&& f)
{
	detail::env_.ptr = env;

	try
	{
		if constexpr (std::is_void_v<R>)
		{
			std::forward<F>(f)();
		}
		else
		{
			return param<JniR, R>::jni(std::forward<F>(f)());
		}
	}
	catch (const vm_exception& e)
	{
		e.get()->throw_();
	}
	catch (...)
	{
		new_object<com::scapix::native_exception>()->throw_();
	}

	if constexpr (!std::is_void_v<R>)
		return {};
}

template <fixed_string ClassName, native_method_info Info>
struct native_method_impl
{
	template <typename JniType, typename CppType, bool AndroidCriticalNative = false>
	struct impl;

	template <typename JniR, typename ...JniArgs, typename R, typename ...Args, typename Class>
	struct impl<JniR(JniArgs...), R(Class::*)(Args...)>
	{
		static param_type<JniR> func(JNIEnv* env, jobject thiz, param_type<JniArgs>... args)
		{
			return invoke<JniR, R>(env, [&]
			{
				decltype(auto) obj = convert_this<Class>(ref<object<ClassName>>(thiz));
				return (obj.*Info.method)(param<JniArgs, Args>::cpp(args)...);
			});
		}
	};

	template <typename JniR, typename ...JniArgs, typename R, typename ...Args>
	struct impl<JniR(JniArgs...), R(Args...)>
	{
		static param_type<JniR> func(JNIEnv* env, jclass clazz, param_type<JniArgs>... args)
		{
			return invoke<JniR, R>(env, [&]
			{
				return Info.method(param<JniArgs, Args>::cpp(args)...);
			});
		}
	};

	template <typename JniR, typename ...JniArgs, typename R, compatible_object<object<ClassName>> This, typename ...Args>
	struct impl<JniR(JniArgs...), R(ref<This>, Args...)>
	{
		static param_type<JniR> func(JNIEnv* env, jobject thiz, param_type<JniArgs>... args)
		{
			return invoke<JniR, R>(env, [&]
			{
				return Info.method(ref<This>(thiz), param<JniArgs, Args>::cpp(args)...);
			});
		}
	};

	template <typename JniR, typename ...JniArgs, typename R, compatible_object<class_> Class, typename ...Args>
	struct impl<JniR(JniArgs...), R(ref<Class>, Args...)>
	{
		static param_type<JniR> func(JNIEnv* env, jclass clazz, param_type<JniArgs>... args)
		{
			return invoke<JniR, R>(env, [&]
			{
				return Info.method(ref<class_>(clazz), param<JniArgs, Args>::cpp(args)...);
			});
		}
	};

	template <typename JniType, typename CppType>
	struct impl<JniType, CppType, true>
	{
		constexpr static auto func = Info.method;
	};

	static constexpr auto get()
	{
		using jni_type = typename decltype(Info)::jni_type;
		using cpp_type = typename decltype(Info)::cpp_type;

		return jni_native_method
		{
			Info.name,
			signature_v<jni_type>,
			impl<jni_type, remove_function_qualifiers_t<std::remove_pointer_t<cpp_type>>, Info.android_critical_native>::func
		};
	}
};

template <fixed_string ClassName, native_method_info ...Methods>
class native_methods
{
public:

	static void register_()
	{
		class_::find_class(ClassName)->register_natives(reinterpret_cast<const JNINativeMethod*>(&methods), sizeof...(Methods));
	}

private:

	native_methods() = delete;

	static constexpr tuple methods = { native_method_impl<ClassName, Methods>::get()... };

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_NATIVE_METHOD_H
