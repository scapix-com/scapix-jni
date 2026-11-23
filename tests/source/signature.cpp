#include <string_view>
#include <scapix/jni/signature.h>
#include <scapix/jni/array.h>
#include <scapix/jni/string.h>
#include <scapix/jni/generics.h>

namespace jni = scapix::jni;

namespace {

template <typename T>
constexpr std::string_view sig = jni::signature_v<T>;

template <typename T>
constexpr std::string_view name = jni::class_name_v<T>;

using integer = jni::object<"java/lang/Integer">;

// generated class: incomplete, known only by object_traits
class incomplete;

} // namespace

template <>
struct scapix::jni::object_traits<incomplete>
{
	static constexpr scapix::fixed_string class_name = "com/example/Incomplete";
	using base_classes = std::tuple<jni::object<>>;
};

static_assert(sig<incomplete> == "Lcom/example/Incomplete;");
static_assert(sig<jni::array<incomplete>> == "[Lcom/example/Incomplete;");
static_assert(sig<jni::parameterized<incomplete, integer>> == "Lcom/example/Incomplete;");

static_assert(sig<void> == "V");
static_assert(sig<jint> == "I");
static_assert(sig<jni::object<>> == "Ljava/lang/Object;");
static_assert(sig<jni::string> == "Ljava/lang/String;");

static_assert(name<jni::array<jint>> == "[I");
static_assert(name<jni::array<jni::string>> == "[Ljava/lang/String;");
static_assert(sig<jni::array<jint>> == "[I");
static_assert(sig<jni::array<jni::string>> == "[Ljava/lang/String;");
static_assert(sig<jni::array<jni::array<jdouble>>> == "[[D");

static_assert(sig<jni::erased<jni::string>> == "Ljava/lang/Object;");
static_assert(sig<jni::parameterized<jni::object<"java/util/List">, integer>> == "Ljava/util/List;");

static_assert(sig<jni::ref<jni::array<jni::string>>(jint, jni::ref<integer>)> == "(ILjava/lang/Integer;)[Ljava/lang/String;");
static_assert(sig<void(jni::ref<jni::erased<jni::string>>, jni::local_ref<jni::string>)> == "(Ljava/lang/Object;Ljava/lang/String;)V");
