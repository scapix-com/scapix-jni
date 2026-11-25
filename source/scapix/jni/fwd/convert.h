/*
	scapix/jni/fwd/convert.h

	Copyright (c) 2019-2026 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_FWD_CONVERT_H
#define SCAPIX_JNI_FWD_CONVERT_H

#include <type_traits>
#include <utility>
#include <scapix/jni/type_traits.h>
#include <scapix/jni/fwd/ref.h>

namespace scapix::jni {

template <typename Jni, typename Cpp>
struct convert;

// convert specialization for (possibly cv/ref qualified, erased) Jni and Cpp

template <typename Jni, typename Cpp>
using convert_t = convert<remove_erased_t<std::remove_cvref_t<Jni>>, std::remove_cvref_t<Cpp>>;

template <typename Jni, typename Cpp>
decltype(auto) convert_jni(Cpp&& cpp)
{
	return convert_t<Jni, Cpp>::jni(std::forward<Cpp>(cpp));
}

template <typename Cpp, typename Jni>
	requires param_type<std::remove_cvref_t<Jni>>
std::remove_cvref_t<Cpp> convert_cpp(Jni&& jni)
{
	// null check only for references: for primitives it is a wasted branch, and loses the sign of -0.0

	if constexpr (ref_type<std::remove_cvref_t<Jni>>)
		if (!jni)
			return {};

	return convert_t<Jni, Cpp>::cpp(std::forward<Jni>(jni));
}

} // namespace scapix::jni

#endif // SCAPIX_JNI_FWD_CONVERT_H
