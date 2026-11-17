/*
	Creates JVM, then runs all Catch2 tests.

	Access violation when running in debugger:

	JVM intentionally raises (and catches) access violation exceptions.

	Configure debugger to ignore such exceptions or continue using these steps:

	Visual Studio: press "Continue".
	XCode: enter into debugger window: "pr h -s false SIGSEGV", then press "Continue".
*/

#include <iostream>
#include <optional>

#include <catch2/catch_session.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <scapix/jni/vm.h>
#include <scapix/jni/local_frame.h>

namespace jni = scapix::jni;

// Each test case runs in its own JNI local frame,
// so local references leaked by one test case don't accumulate.

class local_frame_listener : public Catch::EventListenerBase
{
public:

	using Catch::EventListenerBase::EventListenerBase;

	void testCaseStarting(const Catch::TestCaseInfo&) override
	{
		frame.emplace(16);
	}

	void testCaseEnded(const Catch::TestCaseStats&) override
	{
		frame.reset();
	}

private:

	std::optional<jni::local_frame> frame;

};

CATCH_REGISTER_LISTENER(local_frame_listener)

int main(int argc, char* argv[])
{
	JavaVMOption options[]
	{
		{ const_cast<char*>("-Djava.class.path=" SCAPIX_JNI_JAR_FILE SCAPIX_JNI_CLASS_PATH_SEPARATOR SCAPIX_JNI_TESTS_JAR_FILE) },
		{ const_cast<char*>("-Xcheck:jni") },
	};

	JavaVMInitArgs vm_args
	{
		.version = JNI_VERSION_1_6,
		.nOptions = std::size(options),
		.options = options,
		.ignoreUnrecognized = false,
	};

	try
	{
		jni::create_vm(&vm_args);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << "\n";
		return 1;
	}

	int result = Catch::Session().run(argc, argv);

	jni::destroy_vm();

	return result;
}
