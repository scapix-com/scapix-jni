/*
	scapix/jni/array.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_ARRAY_H
#define SCAPIX_JNI_ARRAY_H

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

template <array_element T>
class array_base : public object<signature_v<array<T>>>
{
public:

	using array_base::object::object;

	jsize size() const { return detail::env()->GetArrayLength(this->handle()); }

};

// object array

template <reference T>
class array<T> : public array_base<T>
{
public:

	using handle_type = typename array::handle_type;

	// Proxy for an array element: reads the element on conversion to ref<T>.

	class const_reference
	{
	public:

		const_reference(const const_reference&) = delete;
		const_reference& operator = (const const_reference&) = delete;

		void operator & () = delete;

		operator ref<T>() const { return get(); }
		ref<T> get() const { return get_element(arr, pos); }
		ref<T> operator -> () const { return get(); }

	protected:

		friend array;

		const_reference(handle_type arr, jsize pos) : arr(arr), pos(pos) {}

		handle_type arr;
		jsize pos;

	};

	// Additionally writes the element on assignment.

	class reference : public const_reference
	{
	public:

		reference& operator = (const reference& other) { return *this = other.get(); }

		template <typename Y, scope Scope>
		reference& operator = (const ref<Y, Scope>& value) { set_element(this->arr, this->pos, value); return *this; }

	private:

		friend array;

		reference(handle_type arr, jsize pos) : const_reference(arr, pos) {}

	};

	template <typename Reference>
	class basic_iterator
	{
	public:

		using difference_type = jsize;
		using value_type = ref<T>;
		using pointer = void;
		using reference = Reference;
		using iterator_category = std::random_access_iterator_tag;

		basic_iterator() = default;

		reference operator *() const { return reference(arr, pos); }
		reference operator ->() const { return reference(arr, pos); }
		reference operator [](difference_type n) const { return reference(arr, pos + n); }

		basic_iterator& operator ++() { ++pos; return *this; }
		basic_iterator& operator --() { --pos; return *this; }

		basic_iterator operator ++(int) { auto temp = *this; ++pos; return temp; }
		basic_iterator operator --(int) { auto temp = *this; --pos; return temp; }

		basic_iterator& operator +=(difference_type n) { pos += n; return *this; }
		basic_iterator& operator -=(difference_type n) { pos -= n; return *this; }

		friend basic_iterator operator + (basic_iterator i, difference_type n) { return i += n; }
		friend basic_iterator operator + (difference_type n, basic_iterator i) { return i += n; }
		friend basic_iterator operator - (basic_iterator i, difference_type n) { return i -= n; }
		friend difference_type operator - (const basic_iterator& a, const basic_iterator& b) { return a.pos - b.pos; }

		// also declares defaulted operator ==
		friend auto operator <=> (const basic_iterator&, const basic_iterator&) = default;

	private:

		friend array;

		basic_iterator(handle_type arr, jsize pos) : arr(arr), pos(pos) {}

		handle_type arr = nullptr;
		jsize pos = 0;

	};

	using value_type = ref<T>;
	using size_type = jsize;
	using difference_type = jsize;
	using pointer = void;
	using const_pointer = void;
	using iterator = basic_iterator<reference>;
	using const_iterator = basic_iterator<const_reference>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	bool empty() const { return this->size() == 0; }

	iterator begin() { return iterator(this->handle(), 0); }
	const_iterator begin() const { return const_iterator(this->handle(), 0); }
	const_iterator cbegin() const { return begin(); }

	iterator end() { return iterator(this->handle(), this->size()); }
	const_iterator end() const { return const_iterator(this->handle(), this->size()); }
	const_iterator cend() const { return end(); }

	reverse_iterator rbegin() { return reverse_iterator(end()); }
	const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	const_reverse_iterator crbegin() const { return rbegin(); }

	reverse_iterator rend() { return reverse_iterator(begin()); }
	const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
	const_reverse_iterator crend() const { return rend(); }

	reference operator[](size_type pos) { return reference(this->handle(), pos); }
	const_reference operator[](size_type pos) const { return const_reference(this->handle(), pos); }

	reference at(size_type pos) { check_range(pos); return (*this)[pos]; }
	const_reference at(size_type pos) const { check_range(pos); return (*this)[pos]; }

	static ref<array> new_object(jsize len, ref<T> init = {})
	{
		return detail::check_result<array>(detail::env()->NewObjectArray(len, object_impl_t<T>::class_object().handle(), init.handle()));
	}

	// for consistency with primitive array

	array& elements() { return *this; }
	const array& elements() const { return *this; }

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

// View of primitive array elements: std::span, which releases the elements on destruction.
// T is const for elements of a const array.

template <typename T, lock Lock, release_mode Mode>
	requires primitive<std::remove_const_t<T>>
class array_elements : public std::span<T>
{
	using span = std::span<T>;
	using primitive_type = std::remove_const_t<T>;
	using api = detail::api::array<primitive_type, Lock>;

public:

	array_elements(const array_elements&) = delete;
	array_elements& operator=(const array_elements&) = delete;

	~array_elements() { reset(Mode); }

	T& at(std::size_t pos) const
	{
		if (pos >= this->size())
			throw std::out_of_range("scapix::jni::array_elements::at");

		return (*this)[pos];
	}

	jboolean is_copy() const { return is_copy_; }

	// Copy elements back to the array, without releasing them.

	void commit() requires (!std::is_const_v<T>) { release_elements(JNI_COMMIT); }

	// After a call to release() or abort() this is an empty span.

	void release() requires (!std::is_const_v<T>) { reset(release_mode::copy); }
	void abort() { reset(release_mode::abort); }

private:

	friend class array<primitive_type>;

	using handle_type = typename array<primitive_type>::handle_type;

	array_elements(handle_type array, jsize size) :
		span(detail::check_result(api::get_array_elements(array, &is_copy_)), static_cast<std::size_t>(size)),
		array_(array)
	{
	}

	void release_elements(jint mode)
	{
		api::release_array_elements(array_, const_cast<primitive_type*>(this->data()), mode);
	}

	void reset(release_mode mode)
	{
		if (this->data())
		{
			release_elements(static_cast<jint>(mode));
			span::operator=(span());
		}
	}

	handle_type array_;
	jboolean is_copy_; // written during base class initialization: must not have an initializer

};

template <primitive T>
class array<T> : public array_base<T>
{
public:

	static ref<array> new_object(jsize len)
	{
		return detail::check_result<array>(detail::api::type<T>::new_array(len));
	}

	template <lock Lock = lock::noncritical, release_mode Mode = release_mode::copy>
	auto elements()
	{
		return elements<Lock, Mode>(this->size());
	}

	template <lock Lock = lock::noncritical, release_mode Mode = release_mode::copy>
	auto elements(jsize size)
	{
		return array_elements<T, Lock, Mode>(this->handle(), size);
	}

	template <lock Lock = lock::noncritical>
	auto elements() const
	{
		return const_elements<Lock>(this->size());
	}

	template <lock Lock = lock::noncritical>
	auto elements(jsize size) const
	{
		return const_elements<Lock>(size);
	}

	template <lock Lock = lock::noncritical>
	auto const_elements() const
	{
		return const_elements<Lock>(this->size());
	}

	template <lock Lock = lock::noncritical>
	auto const_elements(jsize size) const
	{
		return array_elements<const T, Lock, release_mode::abort>(this->handle(), size);
	}

	void get_region(jsize start, jsize len, T* buf) const
	{
		detail::api::type<T>::get_array_region(this->handle(), start, len, buf);
		detail::check_exception();
	}

	void set_region(jsize start, jsize len, const T* buf)
	{
		detail::api::type<T>::set_array_region(this->handle(), start, len, buf);
		detail::check_exception();
	}

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_ARRAY_H
