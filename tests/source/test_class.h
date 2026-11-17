#ifndef SCAPIX_JNI_TESTS_TEST_CLASS_H
#define SCAPIX_JNI_TESTS_TEST_CLASS_H

#include <scapix/jni/object.h>
#include <scapix/jni/string.h>
#include <scapix/jni/array.h>

namespace test {

namespace jni = scapix::jni;

// tests/java/com/scapix/test/TestClass.java

using test_class = jni::object<"com/scapix/test/TestClass">;

} // namespace test

#endif // SCAPIX_JNI_TESTS_TEST_CLASS_H
