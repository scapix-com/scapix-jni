/*
	scapix/jni/object.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

// outside of include guard
#include <scapix/jni/object_impl.h>

#ifndef SCAPIX_JNI_OBJECT_H
#define SCAPIX_JNI_OBJECT_H

#include <tuple>
#include <scapix/jni/fwd/object.h>

namespace scapix::jni {

#include <scapix/core/warning/inaccessible_base.h>

template <fixed_string ClassName, typename ...Bases>
class object : private object_impl<ClassName>, public Bases...
{
	static_assert(ClassName != "java/lang/Object" || sizeof...(Bases) == 0, "java/lang/Object should not specify base classes");

	using impl = object_impl<ClassName>;

public:

	static constexpr fixed_string class_name = ClassName;
	using base_classes = std::tuple<Bases...>;
	using handle_type = handle_type_t<object>;

	using impl::new_object;
	using impl::call_method;
	using impl::call_nonvirtual_method;
	using impl::call_static_method;
	using impl::get_field;
	using impl::set_field;
	using impl::get_static_field;
	using impl::set_static_field;
	using impl::to_reflected_method;
	using impl::to_reflected_static_method;
	using impl::to_reflected_field;
	using impl::to_reflected_static_field;
	using impl::class_object;

protected:

	handle_type handle() const { return static_cast<handle_type>(impl::handle()); }

private:

	template <typename T, scope Scope>
	friend class ref;

	template <fixed_string, typename ...>
	friend class object;

	object(handle_type h) : impl(h), Bases{h}... {}

};

#include <scapix/core/warning/pop.h>

template <fixed_string ClassName>
class object<ClassName> : public object<ClassName, object<>>
{
	using object<ClassName, object<>>::object;
};

template <>
class object<"java/lang/Object"> : private object_impl<"java/lang/Object">
{
public:

	static constexpr fixed_string class_name = "java/lang/Object";
	using base_classes = std::tuple<>;
	using handle_type = handle_type_t<object>;

	using object_impl::new_object;
	using object_impl::call_method;
	using object_impl::call_nonvirtual_method;
	using object_impl::call_static_method;
	using object_impl::get_field;
	using object_impl::set_field;
	using object_impl::get_static_field;
	using object_impl::set_static_field;
	using object_impl::to_reflected_method;
	using object_impl::to_reflected_static_method;
	using object_impl::to_reflected_field;
	using object_impl::to_reflected_static_field;
	using object_impl::class_object;

	template <reference T>
	[[nodiscard]] bool is_instance_of() const
	{
		return is_instance_of(object_impl_t<T>::class_object());
	}

	[[nodiscard]] bool is_instance_of(ref<class_> cls) const noexcept
	{
		return detail::env()->IsInstanceOf(handle(), cls.handle());
	}

	template <reference T>
	[[nodiscard]] ref<T> instance_of() const
	{
		if (is_instance_of<T>())
			return ref<T>(handle());

		return nullptr;
	}

	[[nodiscard]] local_ref<class_> get_object_class() const noexcept
	{
		return local_ref<class_>(detail::env()->GetObjectClass(handle()));
	}

private:

	template <typename T, scope Scope>
	friend class ref;

	template <fixed_string, typename ...>
	friend class object;

	object(handle_type h) : object_impl(h) {}

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_OBJECT_H
