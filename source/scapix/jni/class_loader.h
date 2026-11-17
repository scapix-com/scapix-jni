/*
	scapix/jni/class_loader.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

// outside of include guard
#include <scapix/jni/detail/object_impl.h>

#ifndef SCAPIX_JNI_CLASS_LOADER_H
#define SCAPIX_JNI_CLASS_LOADER_H

#include <scapix/core/immortal.h>
#include <scapix/jni/class.h>
#include <scapix/jni/string.h>

namespace scapix::jni {

/*

In some JVM scenarios (notably on Android), JNI FindClass() fails to load application classes when called from C++ created threads.
More information here: [Why didn’t FindClass find my class?](https://developer.android.com/training/articles/perf-jni#faq:-why-didnt-findclass-find-my-class).
This implements the option "Cache a reference to the ClassLoader object somewhere handy, and issue loadClass calls directly."

Classes are loaded with Class.forName(name, false, loader) instead of ClassLoader.loadClass(name),
because loadClass() does not support array classes (like "[I" or "[Ljava.lang.String;").

*/

class class_loader
{
public:

	static void init()
	{
		loader = object<"java/lang/Thread">::call_static_method<"currentThread", ref<object<"java/lang/Thread">>()>()->call_method<"getContextClassLoader", ref<object<"java/lang/ClassLoader">>()>();
	}

	static local_ref<class_> find_class(const char* name)
	{
		return class_::call_static_method<"forName", ref<class_>(ref<string>, jboolean, ref<object<"java/lang/ClassLoader">>)>(new_object<string>(name), JNI_FALSE, loader);
	}

private:

	inline static immortal<global_ref<object<"java/lang/ClassLoader">>> loader;

};

} // namespace scapix::jni

#endif // SCAPIX_JNI_CLASS_LOADER_H
