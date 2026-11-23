/*
	scapix/jni/object_base.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

// outside of include guard
#include <scapix/jni/detail/object_impl.h>

#ifndef SCAPIX_JNI_OBJECT_BASE_H
#define SCAPIX_JNI_OBJECT_BASE_H

#include <scapix/jni/object.h>

namespace scapix::jni {

#include <scapix/core/warning/inaccessible_base.h>

template <fixed_string ClassName, typename ...Bases>
class object_base : protected detail::object_impl<ClassName>, public Bases...
{
	using impl = detail::object_impl<ClassName>;

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

	object_base(jobject h) : impl(h), Bases{h}... {}
};

#include <scapix/core/warning/pop.h>

} // namespace scapix::jni

#endif // SCAPIX_JNI_OBJECT_BASE_H
