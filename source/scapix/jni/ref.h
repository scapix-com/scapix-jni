/*
	scapix/jni/ref.h

	Copyright (c) 2019-2024 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_REF_H
#define SCAPIX_JNI_REF_H

#include <utility>
#include <scapix/core/type_traits.h>
#include <scapix/jni/type_traits.h>
#include <scapix/jni/signature.h>
#include <scapix/jni/element.h>
#include <scapix/jni/detail/api/ref.h>
#include <scapix/jni/fwd/object.h>
#include <scapix/jni/fwd/class.h>
#include <scapix/jni/fwd/string.h>
#include <scapix/jni/fwd/array.h>
#include <scapix/jni/fwd/ref.h>

namespace scapix::jni {

// redirector

template <typename T>
struct redirector
{
	T* operator -> () && { return &value; }
	const T* operator -> () const && { return &value; }

	T value;
};

// is_ref

template <typename R>
concept is_ref_v = requires (R r)
{
	[] <typename T, scope Scope> (ref<T, Scope>&) {}(r);
};

// canonical_ref

template <typename T>
struct canonical_ref
{
	using type = T;
};

template <typename T, typename Extends, scope Scope>
struct canonical_ref<ref<generic<T, Extends>, Scope>>
{
	using type = ref<T, Scope>;
};

template <typename T>
using canonical_ref_t = typename canonical_ref<T>::type;

// convert

template <typename Jni, typename Cpp>
struct convert;

template <typename Jni, typename Cpp>
decltype(auto) convert_jni(Cpp&& cpp)
{
	return convert<canonical_ref_t<std::remove_cvref_t<Jni>>, std::remove_cvref_t<Cpp>>::jni(std::forward<Cpp>(cpp));
}

template <typename Cpp, typename Jni>
decltype(auto) convert_cpp(Jni&& jni)
{
	if (!jni)
		return std::remove_cvref_t<Cpp>();

	return convert<canonical_ref_t<std::remove_cvref_t<Jni>>, std::remove_cvref_t<Cpp>>::cpp(std::forward<Jni>(jni));
}

template<typename Jni, typename Cpp>
concept has_convert_jni = !is_ref_v<std::remove_cvref_t<Cpp>> && requires(Jni jni, Cpp cpp)
{
	jni = convert<canonical_ref_t<std::remove_cvref_t<Jni>>, std::remove_cvref_t<Cpp>>::jni(cpp);
};

template<typename Jni, typename Cpp>
concept has_convert_cpp = !is_ref_v<std::remove_cvref_t<Cpp>> && requires(Jni jni, Cpp cpp)
{
	cpp = convert<canonical_ref_t<std::remove_cvref_t<Jni>>, std::remove_cvref_t<Cpp>>::cpp(jni);
};

/*

Implementation for owning reference types: local_ref, global_ref, weak_ref.

*/

template <typename T, scope Scope>
class ref
{
public:

	using element_type = element_type_t<T>;
	using handle_type = handle_type_t<element_type>;

	constexpr scope get_scope() { return Scope; }

	ref(std::nullptr_t = nullptr) : object(nullptr) {}
	explicit ref(jobject h) : object(h) {}

	ref(const ref& r) : object(new_ref(r)) {}

	template <object_convertible_to<element_type> Y, scope S>
	ref(const ref<Y, S>& r) : object(new_ref(r)) {}

	ref(ref&& r) noexcept : object(r.release()) {}

	template <object_convertible_to<element_type> Y, scope S>
	ref(ref<Y, S>&& r) : object(Scope == r.get_scope() ? r.release() : new_ref(r)) {}

	~ref()
	{
		if (handle())
			detail::api::ref<Scope>::delete_ref(handle());
	}

	ref& operator = (const ref& r)
	{
		ref(r).swap(*this);
		return *this;
	}

	template <object_convertible_to<element_type> Y, scope S>
	ref& operator = (const ref<Y, S>& r)
	{
		ref(r).swap(*this);
		return *this;
	}

	ref& operator = (ref&& r)
	{
		ref(std::move(r)).swap(*this);
		return *this;
	}

	template <object_convertible_to<element_type> Y, scope S>
	ref& operator = (ref<Y, S>&& r)
	{
		ref(std::move(r)).swap(*this);
		return *this;
	}

	redirector<element_type> operator -> () { return redirector<element_type>(get()); }
	const redirector<element_type> operator -> () const { return redirector<element_type>(get()); }
	element_type operator * () { return get(); }
	const element_type operator * () const { return get(); }

	auto operator [] (jsize i) requires object_array<T> { return get()[i]; }

	explicit operator bool() const { return handle() != nullptr; }
	element_type get() const { return element_type{ handle() }; }

	auto handle() const
	{
		return static_cast<handle_type>(object);
	}

	void reset(jobject h = nullptr)
	{
		ref(h).swap(*this);
	}

	auto release()
	{
		auto temp(handle());
		object = nullptr;
		return temp;
	}

	void swap (ref& r)
	{
		using std::swap;
		swap (object, r.object);
	}

private:

	template <typename Ref>
	static jobject new_ref(const Ref& r)
	{
		return detail::api::ref<Scope>::new_ref(r.handle());
	}

	jobject object;

};

/*

Implementation for (conceptually) non-owning reference type: ref.

In reality takes ownership when moved to, and does not take ownership when copied to.
This allows both zero overhead parameters and safe return of temporaries.

When owning semantics is required, use local_ref, global_ref or weak_ref explicitly.

*/

template <typename T>
class ref<T, scope::generic>
{
public:

	using element_type = element_type_t<T>;
	using handle_type = handle_type_t<element_type>;

	scope get_scope() { return scp; }

	ref(std::nullptr_t = nullptr) : object(nullptr), scp(scope::generic) {}

	explicit(!std::is_same_v<ref, ref<>>) ref(jobject h) : object(h), scp(scope::generic) {}

	ref(const ref& r) : object(r.handle()), scp(scope::generic) {}

	template <object_convertible_to<element_type> Y, scope S>
	ref(const ref<Y, S>& r) : object(r.handle()), scp(scope::generic) {}

	ref(ref&& r) noexcept : object(r.release()), scp(r.get_scope()) {}

	template <object_convertible_to<element_type> Y, scope S>
	ref(ref<Y, S>&& r) : object(r.release()), scp(r.get_scope()) {}

	template <typename X>
		requires has_convert_jni<ref, X>
	ref(X&& x)
		: ref(convert_jni<ref>(std::forward<X>(x)))
	{
	}

	template <typename X>
		requires has_convert_cpp<ref, X>
	operator X() const
	{
		return convert_cpp<X>(*this);
	}

	~ref()
	{
		if (handle())
		{
			if (scp == scope::local)
			{
				detail::api::ref<scope::local>::delete_ref(handle());
			}
			else if (scp == scope::global)
			{
				detail::api::ref<scope::global>::delete_ref(handle());
			}
			else if (scp == scope::weak)
			{
				detail::api::ref<scope::weak>::delete_ref(handle());
			}
		}
	}

	ref& operator = (const ref& r)
	{
		ref(r).swap(*this);
		return *this;
	}

	template <object_convertible_to<element_type> Y, scope S>
	ref& operator = (const ref<Y, S>& r)
	{
		ref(r).swap(*this);
		return *this;
	}

	ref& operator = (ref&& r)
	{
		ref(std::move(r)).swap(*this);
		return *this;
	}

	template <object_convertible_to<element_type> Y, scope S>
	ref& operator = (ref<Y, S>&& r)
	{
		ref(std::move(r)).swap(*this);
		return *this;
	}

	redirector<element_type> operator -> () { return redirector<element_type>(get()); }
	const redirector<element_type> operator -> () const { return redirector<element_type>(get()); }
	element_type operator * () { return get(); }
	const element_type operator * () const { return get(); }

	auto operator [] (jsize i) requires object_array<T> { return get()[i]; }

	explicit operator bool() const { return handle() != nullptr; }
	element_type get() const { return element_type{ handle() }; }

	auto handle() const
	{
		return static_cast<handle_type>(object);
	}

	void reset(jobject h = nullptr)
	{
		ref(h).swap(*this);
	}

	auto release()
	{
		auto temp(handle());
		object = nullptr;
		return temp;
	}

	void swap (ref& r)
	{
		using std::swap;
		swap (object, r.object);
		swap (scp, r.scp);
	}

private:

	template <typename X, typename Y, scope Scope>
	friend ref<X, Scope> static_pointer_cast(ref<Y, Scope>&& r);

	ref(jobject h, scope s) noexcept : object(h), scp(s) {}

private:

	jobject object;
	scope scp;

};

// C++20 Class Template Argument Deduction for Alias Templates
//
// MSVC - VS 2019 16.7
// GCC - 10
// Clang - not supported as of clang 17

#if 0 // __cpp_deduction_guides >= 201907L
template <typename T, scope Scope, scope Scope2>
ref(ref<T, Scope>) -> ref<T, Scope2>;
#endif

template <typename T = object<>>
using local_ref = ref<T, scope::local>;

template <typename T = object<>>
using global_ref = ref<T, scope::global>;

template <typename T = object<>>
using weak_ref = ref<T, scope::weak>;

template <typename T, scope Scope>
inline void swap(ref<T, Scope>& a, ref<T, Scope>& b)
{
	a.swap(b);
}

template <typename T1, scope S1, typename T2, scope S2>
inline bool operator ==(const ref<T1, S1>& a, const ref<T2, S2>& b)
{
	return detail::env()->IsSameObject(a.handle(), b.handle());
}

template <typename T1, scope S1, typename T2, scope S2>
inline bool operator !=(const ref<T1, S1>& a, const ref<T2, S2>& b)
{
	return !(a == b);
}

// global_ref used as static object
// Disable reset() in destructor, as it happens too late for JNI calls.
// Could instead register ref in a global list to call reset() during OnUnload().

template <typename T = object<>>
class static_global_ref : public global_ref<T>
{
public:

	using global_ref<T>::global_ref;

	~static_global_ref()
	{
		global_ref<T>::release();
	}
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_REF_H
