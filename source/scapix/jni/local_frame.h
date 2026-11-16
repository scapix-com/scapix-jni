/*
	scapix/jni/local_frame.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_LOCAL_FRAME_H
#define SCAPIX_JNI_LOCAL_FRAME_H

#include <utility>
#include <scapix/jni/env.h>
#include <scapix/jni/ref.h>
#include <scapix/jni/detail/exception.h>

namespace scapix::jni {

// RAII wrapper for PushLocalFrame()/PopLocalFrame().
//
// Local references created while the frame is active are freed when the frame is popped,
// so owning references to them (local_ref) must be destroyed or released before that.
// To keep one object alive past the frame, move its local_ref to pop().
//
// JNI local frames form a stack: PopLocalFrame() always pops the topmost frame.
// Therefore local_frame objects must be destroyed in reverse order of construction
// (automatic for local variables), and are not copyable or movable.

class local_frame
{
public:

	explicit local_frame(jint capacity)
	{
		detail::check_result(detail::env()->PushLocalFrame(capacity));
		active = true;
	}

	~local_frame()
	{
		pop();
	}

	local_frame(const local_frame&) = delete;
	local_frame& operator = (const local_frame&) = delete;
	local_frame(local_frame&&) = delete;
	local_frame& operator = (local_frame&&) = delete;

	explicit operator bool() const noexcept { return active; }

	// Pop the frame, freeing all local references created in it.

	void pop() noexcept
	{
		if (std::exchange(active, false))
			detail::env()->PopLocalFrame(nullptr);
	}

	// Pop the frame, moving local reference created in it to the previous frame:
	//
	//     return frame.pop(std::move(result));

	template <typename T>
	local_ref<T> pop(local_ref<T>&& result) noexcept
	{
		if (std::exchange(active, false))
			return local_ref<T>(detail::env()->PopLocalFrame(result.release()));

		return result;
	}

private:

	bool active = false;

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_LOCAL_FRAME_H
