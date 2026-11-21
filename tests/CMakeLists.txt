file(GLOB_RECURSE sources CONFIGURE_DEPENDS "source/*")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})

file(GLOB_RECURSE java_sources CONFIGURE_DEPENDS "java/*.java")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${java_sources})

add_jar(scapix_jni_tests_jar SOURCES ${java_sources} OUTPUT_NAME scapix_jni_tests)
get_target_property(scapix_jni_tests_jar_file scapix_jni_tests_jar JAR_FILE)

add_executable(scapix_jni_tests ${sources} ${java_sources})
add_dependencies(scapix_jni_tests scapix_jni_tests_jar)
target_compile_definitions(scapix_jni_tests PRIVATE SCAPIX_JNI_TESTS_JAR_FILE="${scapix_jni_tests_jar_file}")

# Catch2 is built with this standard: std::string_view support must match the tests.
set(CMAKE_CXX_STANDARD 20)
find_package(Catch2 REQUIRED)
target_link_libraries(scapix_jni_tests PRIVATE scapix::jni Catch2::Catch2)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT scapix_jni_tests)

add_test(NAME scapix_jni_tests COMMAND scapix_jni_tests)

# Windows: jvm.dll is not on PATH by default.

set(run_tests_env)

if(WIN32)
    find_path(JVM_DLL_PATH NAMES "jvm.dll" PATHS "${JAVA_INCLUDE_PATH}/../jre/bin/server" "${JAVA_INCLUDE_PATH}/../bin/server" NO_DEFAULT_PATH)

    if(JVM_DLL_PATH)
        set_target_properties(scapix_jni_tests PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;${JVM_DLL_PATH}")
        set_tests_properties(scapix_jni_tests PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${JVM_DLL_PATH}")
        set(run_tests_env --modify "PATH=path_list_prepend:${JVM_DLL_PATH}")
    endif()
endif()

add_custom_target(scapix_jni_run_tests
    COMMAND ${CMAKE_COMMAND} -E env ${run_tests_env} $<TARGET_FILE:scapix_jni_tests>
    DEPENDS scapix_jni_tests
    USES_TERMINAL
)
