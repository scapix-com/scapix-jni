/*
	scapix/jni/monitor.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_MONITOR_H
#define SCAPIX_JNI_MONITOR_H

#include <utility>
#include <cassert>
#include <scapix/jni/ref.h>
#include <scapix/jni/detail/exception.h>

namespace scapix::jni {

// RAII wrapper for MonitorEnter()/MonitorExit().
//
// Monitor must be exited on the same thread it was entered on.

class monitor
{
public:

	// Generally, caller is responsible to ensure the reference (not only the object) is alive while monitor is active.
	// But if an owning reference is moved here, monitor will take ownership.

	explicit monitor(ref<> obj)
		: object(std::move(obj))
	{
		assert(object);
		detail::check_result(detail::env()->MonitorEnter(object.handle()));
	}

	monitor(const monitor&) = delete;
	monitor& operator = (const monitor&) = delete;

	monitor(monitor&& source) noexcept
		: object(std::move(source.object))
	{
	}

	monitor& operator = (monitor&& source) noexcept
	{
		monitor(std::move(source)).swap(*this);
		return *this;
	}

	~monitor()
	{
		if (object)
		{
			[[maybe_unused]] auto result = detail::env()->MonitorExit(object.handle());
			assert(result == 0);
		}
	}

	explicit operator bool() const noexcept { return bool(object); }
	ref<> get() const noexcept { return object; }

	void reset() noexcept
	{
		monitor().swap(*this);
	}

	void swap(monitor& source) noexcept
	{
		using std::swap;
		swap(object, source.object);
	}

private:

	monitor() noexcept = default;

	ref<> object;

};

inline void swap(monitor& a, monitor& b) noexcept
{
	a.swap(b);
}

} // namespace scapix::jni

#endif // SCAPIX_JNI_MONITOR_H
