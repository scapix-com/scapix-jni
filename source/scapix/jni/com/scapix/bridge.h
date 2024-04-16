/*
	scapix/jni/com/scapix/bridge.h

	Copyright (c) 2019-2024 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_COM_SCAPIX_BRIDGE_H
#define SCAPIX_JNI_COM_SCAPIX_BRIDGE_H

#include <memory>
#include <utility>
#include <type_traits>
#include <scapix/jni/object.h>
#include <scapix/jni/convert.h>
#include <scapix/jni/fwd/native_method.h>

namespace scapix::jni {
namespace com::scapix {

namespace cpp { class object_base; }

class bridge_;

class bridge : public object<"com/scapix/Bridge">
{
public:

	using nop = object<"com/scapix/Bridge$Nop">;

	void set_ptr(cpp::object_base* p) { set_field<"ptr", jlong>(reinterpret_cast<jlong>(p)); }
	cpp::object_base* get_ptr() const { return reinterpret_cast<cpp::object_base*>(get_field<"ptr", jlong>()); }

};

template <fixed_string ClassName, typename T>
class bridge_object : public object<ClassName, bridge>
{
	using base = object<ClassName, bridge>;

public:

	static local_ref<bridge_object> new_object()
	{
		return jni::new_object<bridge_object, void(ref<typename base::nop>)>(nullptr);
	}

	T* get_ptr() const { return static_cast<T*>(base::get_ptr()); }

};

namespace cpp {

class object_base
{
protected:

	object_base() = default;
	object_base(const object_base&) {}
	object_base(object_base&&) = default;
	object_base& operator =(const object_base&) { return *this; }
	object_base& operator =(object_base&&) = default;

private:

	template <fixed_string ClassName, typename T, typename ...Args>
	friend void init(ref<bridge_object<ClassName, T>> thiz, Args... args);

	template <typename Jni, typename Cpp>
	friend struct jni::convert;

	friend class com::scapix::bridge_;

	void attach(ref<bridge> obj, std::shared_ptr<object_base> shared_this)
	{
		assert(!wrapper);
		assert(!self);

		wrapper = std::move(obj);
		self = std::move(shared_this);

		wrapper->set_ptr(this);
	}

	// to do: with indirect inheritance support,
	// wrappers should depend on actual object type.

	template <fixed_string ClassName, typename T>
	local_ref<bridge_object<ClassName, T>> get_ref(std::shared_ptr<T> shared_this)
	{
		local_ref<bridge_object<ClassName, T>> local(static_pointer_cast<bridge_object<ClassName, T>>(wrapper));

		if (!local)
		{
			local = new_object<bridge_object<ClassName, T>>();
			attach(local, std::move(shared_this));
		}

		return local;
	}

	const std::shared_ptr<object_base>& scapix_shared()
	{
		assert(self);
		return self;
	}

	void finalize()
	{
		wrapper.reset();
		self.reset(); // might destroy this object
	}

	weak_ref<bridge> wrapper;
	std::shared_ptr<object_base> self;

};

// to do: inheritance should be private

template <typename>
class object : public object_base
{
protected:

	object() = default;
	object(const object&) = default;
	object(object&&) = default;
	object& operator =(const object&) = default;
	object& operator =(object&&) = default;

};

// No 'universal reference' arguments, as types are explicitly specified, not deduced.

template <fixed_string ClassName, typename T, typename ...Args>
void init(ref<bridge_object<ClassName, T>> thiz, Args... args)
{
	std::shared_ptr<object_base> obj = std::make_shared<T>(std::forward<Args>(args)...);
	object_base* ptr = obj.get();
	ptr->attach(std::move(thiz), std::move(obj));
}

} // namespace cpp

class bridge_
{
public:

	using native_methods = jni::native_methods
	<
		bridge::class_name,
		native_method("finalize", &cpp::object_base::finalize)
	>;

};

} // namespace com::scapix

template <std::derived_from<com::scapix::cpp::object_base> T, fixed_string ClassName>
T& convert_this(ref<object<ClassName>> x)
{
	return *ref<com::scapix::bridge_object<ClassName, T>>(x)->get_ptr();
}

template <fixed_string ClassName, std::derived_from<com::scapix::cpp::object_base> T>
struct convert<ref<object<ClassName>>, std::shared_ptr<T>>
{
	static std::shared_ptr<T> cpp(ref<com::scapix::bridge_object<ClassName, T>> v)
	{
		if (!v)
			return nullptr;

		return std::static_pointer_cast<T>(v->get_ptr()->scapix_shared());
	}

	static ref<com::scapix::bridge_object<ClassName, T>> jni(std::shared_ptr<T> v)
	{
		if (!v)
			return nullptr;

		auto p = v.get();
		return p->template get_ref<ClassName>(std::move(v));
	}
};

} // namespace scapix::jni

#endif // SCAPIX_JNI_COM_SCAPIX_BRIDGE_H
