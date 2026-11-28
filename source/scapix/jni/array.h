/*
	scapix/jni/array.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_ARRAY_H
#define SCAPIX_JNI_ARRAY_H

#include <cassert>
#include <compare>
#include <cstddef>
#include <iterator>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <scapix/jni/object.h>
#include <scapix/jni/ref.h>
#include <scapix/jni/lock.h>
#include <scapix/jni/signature.h>
#include <scapix/jni/detail/api/type.h>
#include <scapix/jni/detail/api/array.h>

namespace scapix::jni {

template <java_type T>
class array_base : public object<class_name_v<array<T>>>
{
public:

	using array_base::object::object;

	jsize size() const { return detail::env()->GetArrayLength(this->handle()); }
	bool empty() const { return size() == 0; }

};

// object array

template <object_type T>
class array<T> : public array_base<T>
{
public:

	using handle_type = typename array::handle_type;

	// Proxy for an array element: conversion to ref<T> reads it, assignment writes it, one JNI call each.

	class reference
	{
	public:

		reference(const reference&) = delete;
		const reference& operator = (const reference& other) const { return *this = other.get(); }

		template <typename Y, scope Scope>
		const reference& operator = (const ref<Y, Scope>& value) const { set_element(arr, pos, value); return *this; }

		void operator & () = delete;

		operator ref<T>() const { return get(); }
		ref<T> get() const { return get_element(arr, pos); }
		ref<T> operator -> () const { return get(); }

	private:

		friend array;

		reference(handle_type arr, jsize pos) : arr(arr), pos(pos) {}

		handle_type arr;
		jsize pos;

	};

	// std::random_access_iterator, std::indirectly_writable.

	class iterator
	{
	public:

		using difference_type = jsize;
		using value_type = ref<T>;
		using pointer = void;
		using reference = array::reference;
		using iterator_category = std::random_access_iterator_tag;

		iterator() = default;

		reference operator *() const { return reference(arr, pos); }
		reference operator ->() const { return reference(arr, pos); }
		reference operator [](difference_type n) const { return reference(arr, pos + n); }

		iterator& operator ++() { ++pos; return *this; }
		iterator& operator --() { --pos; return *this; }

		iterator operator ++(int) { auto temp = *this; ++pos; return temp; }
		iterator operator --(int) { auto temp = *this; --pos; return temp; }

		iterator& operator +=(difference_type n) { pos += n; return *this; }
		iterator& operator -=(difference_type n) { pos -= n; return *this; }

		friend iterator operator + (iterator i, difference_type n) { return i += n; }
		friend iterator operator + (difference_type n, iterator i) { return i += n; }
		friend iterator operator - (iterator i, difference_type n) { return i -= n; }
		friend difference_type operator - (const iterator& a, const iterator& b) { return a.pos - b.pos; }

		// also declares defaulted operator ==
		friend auto operator <=> (const iterator&, const iterator&) = default;

	private:

		friend array;

		iterator(handle_type arr, jsize pos) : arr(arr), pos(pos) {}

		handle_type arr = nullptr;
		jsize pos = 0;

	};

	using value_type = ref<T>;
	using size_type = jsize;
	using difference_type = jsize;
	using pointer = void;
	using reverse_iterator = std::reverse_iterator<iterator>;

	iterator begin() const { return iterator(this->handle(), 0); }
	iterator end() const { return iterator(this->handle(), this->size()); }

	reverse_iterator rbegin() const { return reverse_iterator(end()); }
	reverse_iterator rend() const { return reverse_iterator(begin()); }

	reference operator[](size_type pos) const { return reference(this->handle(), pos); }
	reference at(size_type pos) const { check_range(pos); return (*this)[pos]; }

	static ref<array> new_array(jsize len, ref<T> init = {})
	{
		return detail::check_result<array>(detail::env()->NewObjectArray(len, raw_object<T>::class_object().handle(), init.handle()));
	}

private:

	void check_range(size_type pos) const
	{
		if (pos < 0 || pos >= this->size())
			throw std::out_of_range("scapix::jni::array::at");
	}

	static ref<T> get_element(handle_type arr, jsize index)
	{
		jobject element = detail::env()->GetObjectArrayElement(arr, index);
		detail::check_exception();
		return local_ref<T>(element);
	}

	static void set_element(handle_type arr, jsize index, ref<T> value)
	{
		detail::env()->SetObjectArrayElement(arr, index, value.handle());
		detail::check_exception();
	}

};

// primitive array

// View of primitive array elements: std::span, released on destruction.
// Does not keep the array alive.
//
// Read-only view (T is const) is released without copying back (JNI_ABORT).
// Writable view is released copying back (mode 0).

template <typename T, lock Lock>
	requires primitive<std::remove_const_t<T>>
class array_elements : public std::span<T>
{
	using span = std::span<T>;
	using api = detail::api::array<typename span::value_type, Lock>;

	static constexpr jint release_mode = std::is_const_v<T> ? JNI_ABORT : 0;

public:

	using value_type = typename span::value_type;

	array_elements(const array_elements&) = delete;
	array_elements& operator=(const array_elements&) = delete;

	~array_elements() { release(); }

	T& at(std::size_t pos) const
	{
		if (pos >= this->size())
			throw std::out_of_range("scapix::jni::array_elements::at");

		return (*this)[pos];
	}

	jboolean is_copy() const { return is_copy_; }

	// Copy elements back to the array without releasing them (JNI_COMMIT).

	void commit() requires (!std::is_const_v<T>) { release_elements(JNI_COMMIT); }

	// Release elements as the destructor would, leaving an empty span.

	void release() { release(release_mode); }

	// Release elements without copying back (JNI_ABORT), leaving an empty span.
	// Does not undo writes: they are already visible to Java if the elements are not a copy.

	void abort() requires (!std::is_const_v<T>) { release(JNI_ABORT); }

private:

	friend class array<value_type>;

	void release(jint mode)
	{
		if (this->data())
		{
			release_elements(mode);
			span::operator=(span());
		}
	}

	using handle_type = typename array<value_type>::handle_type;

	array_elements(handle_type array, jsize size) :
		span(detail::check_result(api::get_array_elements(array, &is_copy_)), static_cast<std::size_t>(size)),
		array_(array)
	{
	}

	void release_elements(jint mode)
	{
		api::release_array_elements(array_, const_cast<value_type*>(this->data()), mode);
	}

	handle_type array_;
	jboolean is_copy_; // written during base class initialization: must not have an initializer

};

template <primitive T>
class array<T> : public array_base<T>
{
public:

	static ref<array> new_array(jsize len)
	{
		return detail::check_result<array>(detail::api::type<T>::new_array(len));
	}

	// Read-only access is the default: writes are requested explicitly, with writable_elements() or set_region().
	// Constness of the array does not matter: like ref, array is a handle (shallow const).

	template <lock Lock = lock::noncritical>
	array_elements<const T, Lock> elements() const
	{
		return array_elements<const T, Lock>(this->handle(), this->size());
	}

	template <lock Lock = lock::noncritical>
	array_elements<const T, Lock> elements(jsize size) const
	{
		assert(size >= 0 && size <= this->size());
		return array_elements<const T, Lock>(this->handle(), size);
	}

	template <lock Lock = lock::noncritical>
	array_elements<T, Lock> writable_elements() const
	{
		return array_elements<T, Lock>(this->handle(), this->size());
	}

	template <lock Lock = lock::noncritical>
	array_elements<T, Lock> writable_elements(jsize size) const
	{
		assert(size >= 0 && size <= this->size());
		return array_elements<T, Lock>(this->handle(), size);
	}

	// Copy buf.size() elements, starting at start, from the array to buf.

	void get_region(jsize start, std::span<T> buf) const
	{
		detail::api::type<T>::get_array_region(this->handle(), start, static_cast<jsize>(buf.size()), buf.data());
		detail::check_exception();
	}

	// Copy buf.size() elements from buf to the array, starting at start.

	void set_region(jsize start, std::span<const T> buf) const
	{
		detail::api::type<T>::set_array_region(this->handle(), start, static_cast<jsize>(buf.size()), buf.data());
		detail::check_exception();
	}

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_ARRAY_H
