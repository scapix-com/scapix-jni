/*
	scapix/jni/range.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_RANGE_H
#define SCAPIX_JNI_RANGE_H

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <scapix/jni/object.h>
#include <scapix/jni/ref.h>
#include <scapix/jni/cast.h>
#include <scapix/jni/element.h>

namespace scapix::jni {

/*

C++ input range over java.lang.Iterable (any java.util.Collection) or java.util.Iterator:

	for (ref<string> s : jni::range<string>(list))
		...

	for (std::string s : jni::range<string>(list)) // converted
		...

Java type must be known statically: object type must be (or declare as base class)
java.lang.Iterable or java.util.Iterator. For raw types (like object<"java/util/ArrayList">),
declare base class, or use static_pointer_cast<java_lang_iterable>().

Element type E defaults to the first type parameter of generic_type<T, E, ...>, otherwise object<>.
Elements are cast to E without a check (Java generics are erased).

Dereferencing the iterator returns a non-owning ref<E>, valid until the iterator is incremented:
copy it to local_ref<E> or global_ref<E> to keep it longer.
Only one local reference to an element is alive at a time, regardless of collection size.

null Iterable or Iterator is an empty range.

*/

using java_lang_iterable = object<"java/lang/Iterable">;
using java_util_iterator = object<"java/util/Iterator">;

template <reference E = object<>>
class iterable_range;

template <reference E = object<>>
class iterator_range;

template <reference E = object<>>
class range_iterator
{
public:

	using value_type = ref<E>;
	using reference = ref<E>;
	using difference_type = std::ptrdiff_t;
	using iterator_concept = std::input_iterator_tag;

	range_iterator() = default;

	range_iterator(range_iterator&&) = default;
	range_iterator& operator = (range_iterator&&) = default;

	reference operator * () const noexcept { return current; }

	range_iterator& operator ++ ()
	{
		advance();
		return *this;
	}

	void operator ++ (int)
	{
		advance();
	}

	friend bool operator == (const range_iterator& i, std::default_sentinel_t) noexcept
	{
		return !i.iterator;
	}

private:

	friend class iterable_range<E>;
	friend class iterator_range<E>;

	explicit range_iterator(local_ref<java_util_iterator>&& i) : iterator(std::move(i))
	{
		if (iterator)
			advance();
	}

	void advance()
	{
		if (iterator->call_method<"hasNext", jboolean()>())
			current = static_pointer_cast<E>(iterator->call_method<"next", ref<>()>());
		else
			*this = range_iterator(); // end
	}

	local_ref<java_util_iterator> iterator;
	local_ref<E> current;

};

// Range over java.lang.Iterable: each begin() creates new Java iterator.

template <reference E>
class iterable_range
{
public:

	using iterator = range_iterator<E>;

	explicit iterable_range(ref<java_lang_iterable> iterable) : iterable(std::move(iterable)) {}

	iterator begin() const
	{
		return iterator(iterable ? iterable->call_method<"iterator", ref<java_util_iterator>()>() : nullptr);
	}

	std::default_sentinel_t end() const noexcept { return {}; }

private:

	ref<java_lang_iterable> iterable;

};

// Range over java.util.Iterator: single pass, begin() may only be called once.

template <reference E>
class iterator_range
{
public:

	using iterator = range_iterator<E>;

	explicit iterator_range(ref<java_util_iterator> iterator) : source(std::move(iterator)) {}

	iterator begin()
	{
		return iterator(local_ref<java_util_iterator>(std::move(source)));
	}

	std::default_sentinel_t end() const noexcept { return {}; }

private:

	ref<java_util_iterator> source;

};

namespace detail {

template <typename R>
struct range_element
{
	using type = object<>;
};

template <typename T, typename E, typename ...Params, scope Scope>
struct range_element<ref<generic_type<T, E, Params...>, Scope>>
{
	using type = E;
};

template <typename E, typename R>
using range_element_t = std::conditional_t<std::is_void_v<E>, typename range_element<std::remove_cvref_t<R>>::type, E>;

template <typename R, typename J>
concept ref_to = is_ref<std::remove_cvref_t<R>> && object_convertible_to<typename std::remove_cvref_t<R>::element_type, J>;

} // namespace detail

// E = void: deduce element type, see above.
// r is taken over when moved (temporary), otherwise referenced.
// If object is both Iterable and Iterator, it is iterated as Iterable.

template <typename E = void, detail::ref_to<java_lang_iterable> R>
iterable_range<detail::range_element_t<E, R>> range(R&& r)
{
	return iterable_range<detail::range_element_t<E, R>>(static_pointer_cast<java_lang_iterable>(std::forward<R>(r)));
}

template <typename E = void, detail::ref_to<java_util_iterator> R>
	requires (!detail::ref_to<R, java_lang_iterable>)
iterator_range<detail::range_element_t<E, R>> range(R&& r)
{
	return iterator_range<detail::range_element_t<E, R>>(static_pointer_cast<java_util_iterator>(std::forward<R>(r)));
}

} // namespace scapix::jni

#endif // SCAPIX_JNI_RANGE_H
