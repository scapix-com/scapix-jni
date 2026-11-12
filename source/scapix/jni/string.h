/*
	scapix/jni/string.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

// outside of include guard
#include <scapix/jni/object_impl.h>

#ifndef SCAPIX_JNI_STRING_H
#define SCAPIX_JNI_STRING_H

#include <cassert>
#include <concepts>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <string_view>
#include <scapix/jni/object.h>
#include <scapix/jni/lock.h>
#include <scapix/jni/detail/api/string.h>

namespace scapix::jni {

// View of string characters: std::span, which releases the characters on destruction.
// Char is jchar (UTF-16) or char (JNI modified UTF-8).

template <typename Char, lock Lock>
class string_chars : public std::span<const Char>
{
	static_assert(!std::same_as<Char, char> || Lock != lock::critical, "Unsupported parameter combination: <char, critical>");

	using span = std::span<const Char>;
	using api = detail::api::string<Char, Lock>;

public:

	string_chars(const string_chars&) = delete;
	string_chars& operator=(const string_chars&) = delete;

	~string_chars() { release(); }

	const Char& at(std::size_t pos) const
	{
		if (pos >= this->size())
			throw std::out_of_range("scapix::jni::string_chars::at");

		return (*this)[pos];
	}

	jboolean is_copy() const { return is_copy_; }

	std::basic_string_view<Char> view() const
	{
		return std::basic_string_view<Char>(this->data(), this->size());
	}

	// After a call to release() this is an empty span.

	void release()
	{
		if (this->data())
		{
			api::release_chars(string_, this->data());
			span::operator=(span());
		}
	}

private:

	friend class string;

	string_chars(jstring string, jsize size) :
		span(detail::check_result(api::get_chars(string, &is_copy_)), static_cast<std::size_t>(size)),
		string_(string)
	{
	}

	jstring string_;
	jboolean is_copy_; // written during base class initialization: must not have an initializer

};

class string : public object<"java/lang/String", object<"java/lang/Comparable">>
{
public:

	static local_ref<string> new_object(const jchar* buf, jsize len)
	{
		return detail::check_result<string>(detail::env()->NewString(buf, len));
	}

	static local_ref<string> new_object(const char* buf)
	{
		return detail::check_result<string>(detail::env()->NewStringUTF(buf));
	}

	template <typename Char = jchar>
	jsize length() const
	{
		return detail::api::string<Char>::length(handle());
	}

	template <typename Char = jchar, lock Lock = lock::noncritical>
	string_chars<Char, Lock> chars() const
	{
		return string_chars<Char, Lock>(handle(), length<Char>());
	}

	template <typename Char = jchar, lock Lock = lock::noncritical>
	string_chars<Char, Lock> chars(jsize size) const
	{
		assert(size >= 0 && size <= length<Char>());
		return string_chars<Char, Lock>(handle(), size);
	}

	template <typename Char>
	void get_region(jsize start, jsize len, Char* buf) const
	{
		detail::api::string<Char>::get_region(handle(), start, len, buf);
		detail::check_exception();
	}

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_STRING_H
