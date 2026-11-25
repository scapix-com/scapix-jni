/*
	scapix/jni/object_base.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

// outside of include guard
#include <scapix/jni/detail/object_impl.h>

#ifndef SCAPIX_JNI_OBJECT_BASE_H
#define SCAPIX_JNI_OBJECT_BASE_H

#include <type_traits>
#include <utility>
#include <scapix/jni/object.h>

namespace scapix::jni {

#include <scapix/core/warning/inaccessible_base.h>

// object_base
//
// Base of generated classes. All members are protected, used only by generated code.
//
// object_base<ClassName, Bases...> derives from object_base<ClassName> (no Java base classes),
// which holds the members. Generic classes, whose object_base<ClassName, Bases...> depends on
// type parameters, use object_base<ClassName> as non-dependent base to qualify member calls:
//
// template <typename E>
// class ArrayList : public jni::object_base<"java/util/ArrayList", List<E>>
// {
//     using object_base = jni::object_base<"java/util/ArrayList">;
//
//     jint size() { return object_base::call_method<"size", jint>(); }
// };

template <fixed_string ClassName, typename ...Bases>
class object_base;

template <fixed_string ClassName>
class object_base<ClassName> : private detail::object_impl<ClassName>
{
	using impl = detail::object_impl<ClassName>;

protected:

	using impl::get_field;
	using impl::set_field;
	using impl::get_static_field;
	using impl::set_static_field;

	// These variants detect signature from parameter types.

	template <typename ...Args>
	static local_ref<object<ClassName>> new_object(Args&&... args)
	{
		return impl::template new_object<void(std::remove_reference_t<Args>...)>(std::forward<Args>(args)...);
	}

	template <fixed_string Name, typename R, typename ...Args>
	R call_method(Args&&... args) const
	{
		return impl::template call_method<Name, R(std::remove_reference_t<Args>...)>(std::forward<Args>(args)...);
	}

	template <fixed_string Name, typename R, typename ...Args>
	static R call_static_method(Args&&... args)
	{
		return impl::template call_static_method<Name, R(std::remove_reference_t<Args>...)>(std::forward<Args>(args)...);
	}

private:

	template <typename T, scope Scope>
	friend class ref;

	template <fixed_string, typename ...>
	friend class object_base;

	object_base(jobject h) : impl(h) {}
};

template <fixed_string ClassName, typename Base, typename ...Bases>
class object_base<ClassName, Base, Bases...> : protected object_base<ClassName>, public Base, public Bases...
{
	using impl = object_base<ClassName>;

protected:

	using impl::new_object;
	using impl::call_method;
	using impl::call_static_method;
	using impl::get_field;
	using impl::set_field;
	using impl::get_static_field;
	using impl::set_static_field;

private:

	template <typename T, scope Scope>
	friend class ref;

	template <fixed_string, typename ...>
	friend class object_base;

	object_base(jobject h) : impl(h), Base{h}, Bases{h}... {}
};

#include <scapix/core/warning/pop.h>

} // namespace scapix::jni

#endif // SCAPIX_JNI_OBJECT_BASE_H
