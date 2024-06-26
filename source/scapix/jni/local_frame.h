/*
	scapix/jni/local_frame.h

	Copyright (c) 2019-2024 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_LOCAL_FRAME_H
#define SCAPIX_JNI_LOCAL_FRAME_H

#include <utility>
#include <scapix/jni/env.h>

namespace scapix::jni {

class local_frame
{
public:

	local_frame(jint capacity)
	{
		frame = detail::env()->PushLocalFrame(capacity);
	}

	~local_frame()
	{
		if (frame == 0)
			detail::env()->PopLocalFrame(0);
	}

	jobject pop(jobject result = 0)
	{
		if (frame == 0)
		{
			frame = -1;
			return detail::env()->PopLocalFrame(result);
		}

		return 0;
	}

	local_frame(const local_frame&) = delete;
	local_frame& operator = (const local_frame&) = delete;

	local_frame(local_frame&& source) :
		frame(source.frame)
	{
		source.frame = -1;
	}

	local_frame& operator = (local_frame&& source)
	{
		local_frame(std::move(source)).swap(*this);
		return *this;
	}

	void swap(local_frame& source)
	{
		using std::swap;
		swap(frame, source.frame);
	}

private:

	jint frame;

};

inline void swap(local_frame& a, local_frame& b)
{
	a.swap(b);
}

} // namespace scapix::jni

#endif // SCAPIX_JNI_LOCAL_FRAME_H
