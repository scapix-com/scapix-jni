/*
	scapix/jni/compare.h

	Copyright (c) 2019-2024 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_COMPARE_H
#define SCAPIX_JNI_COMPARE_H

#include <functional>
#include <scapix/jni/ref.h>
#include <scapix/jni/type_traits.h>
#include <scapix/jni/object.h>

namespace scapix::jni {

// Identity (reference) equality:
// JNI IsSameObject()

template <reference T>
struct ref_identity_equal
{
	using is_transparent = void;

	template <object_convertible_to<T> Y1, scope S1, object_convertible_to<T> Y2, scope S2>
	bool operator()(const ref<Y1, S1>& r1, const ref<Y2, S2>& r2) const
	{
		return r1 == r2;
	}
};

// Identity (reference) hash:
// java.lang.System.identityHashCode()
// non-virtual Object.hashCode()

template <reference T>
struct ref_identity_hash
{
	using is_transparent = void;

	template <object_convertible_to<T> Y, scope S>
	std::size_t operator()(const ref<Y, S>& r) const
	{
		return object<"java/lang/System">::call_static_method<"identityHashCode", jint(ref<>)>(r);
	}
};

// Value (content) equality:
// java.lang.Object.equals()
// java.util.Objects.equals()

template <reference T>
struct ref_value_equal
{
	using is_transparent = void;

	template <object_convertible_to<T> Y1, scope S1, object_convertible_to<T> Y2, scope S2>
	bool operator()(const ref<Y1, S1>& r1, const ref<Y2, S2>& r2) const
	{
		return object<"java/util/Objects">::call_static_method<"equals", jboolean(ref<>, ref<>)>(r1, r2);
	}
};

// Value (content) hash:
// java.lang.Object.hashCode()
// java.util.Objects.hashCode()

template <reference T>
struct ref_value_hash
{
	using is_transparent = void;

	template <object_convertible_to<T> Y, scope S>
	std::size_t operator()(const ref<Y, S>& r) const
	{
		//return object<"java/util/Objects">::call_static_method<"hashCode", jint(ref<>)>(r);

		if (!r)
			return 0;

		return ref<>(r)->template call_method<"hashCode", jint()>();
	}
};

// ref_less
// java.lang.Comparable.compareTo()

template <object_convertible_to<object<"java/lang/Comparable">> T>
struct ref_less
{
	using is_transparent = void;

	template <object_convertible_to<T> Y1, scope S1, object_convertible_to<T> Y2, scope S2>
	bool operator()(const ref<Y1, S1>& r1, const ref<Y2, S2>& r2) const
	{
		// null ref is less then any non-null ref.

		if (!r2)
			return false;

		if (!r1)
			return true;

		return ref<object<"java/lang/Comparable">>(r1)->template call_method<"compareTo", jint(ref<>)>(r2) < 0;
	}
};

} // namespace scapix::jni

// Specializations of std::equal_to and std::hash.
//
// By default, ref uses identity (reference) equality and hash:
// std::unordered_map<jni::global_ref<JavaClass>, value_type> map;
//
// You can also specify value (content) equality and hash:
// std::unordered_map<jni::global_ref<JavaClass>, value_type, jni::ref_value_hash<JavaClass>, jni::ref_value_equal<JavaClass>> map;

template <scapix::jni::reference T, scapix::jni::scope S>
struct std::equal_to<scapix::jni::ref<T, S>> : scapix::jni::ref_identity_equal<T>
{
	using is_transparent = void;
};

template <scapix::jni::reference T, scapix::jni::scope S>
struct std::hash<scapix::jni::ref<T, S>> : scapix::jni::ref_identity_hash<T>
{
	using is_transparent = void;
};

// Specialization of std::less.
//
// Only for classes implementing java.lang.Comparable:
// std::map<jni::global_ref<JavaClass>, value_type> map;

template <scapix::jni::object_convertible_to<scapix::jni::object<"java/lang/Comparable">> T, scapix::jni::scope S>
struct std::less<scapix::jni::ref<T, S>> : scapix::jni::ref_less<T>
{
	using is_transparent = void;
};

#endif // SCAPIX_JNI_COMPARE_H
