cmake_minimum_required(VERSION 3.28...4.5)

project(scapix-jni VERSION 2.8.4 LANGUAGES CXX)

option(SCAPIX_JNI_CACHE_CLASS_LOADER "Load classes with cached ClassLoader instead of JNI FindClass()" ON)
option(SCAPIX_JNI_AUTO_ATTACH_THREAD "Automatically attach native threads to JVM" ON)
option(SCAPIX_JNI_STANDARD_UTF8 "Convert strings using standard UTF-8 instead of JNI modified UTF-8" ON)
option(SCAPIX_JNI_BUILD_TESTS "Build scapix-jni tests" ${PROJECT_IS_TOP_LEVEL})

find_package(ScapixCore 2.6 REQUIRED)

if(ANDROID)

    # SCAPIX_JNI_PLATFORM_CLASSES
    # For Android, this is the path to the android.jar file (highest installed platform).

    file(GLOB platform_dirs "${CMAKE_ANDROID_NDK}/../../platforms/android-*")
    list(SORT platform_dirs COMPARE NATURAL)

    if(platform_dirs)
        list(GET platform_dirs -1 platform_classes)
        cmake_path(APPEND platform_classes "android.jar")
        cmake_path(NORMAL_PATH platform_classes)
    endif()

    set(SCAPIX_JNI_PLATFORM_CLASSES "${platform_classes}" CACHE FILEPATH "Path to Java platform classes")

else()

    # Function scope restores CMAKE_FIND_FRAMEWORK.
    # Imported targets JNI::* remain visible after the function returns.

    function(scapix_jni_find_jni)
        set(CMAKE_FIND_FRAMEWORK LAST)
        find_package(JNI REQUIRED COMPONENTS JVM)
    endfunction()

    scapix_jni_find_jni()

    find_package(Java REQUIRED COMPONENTS Development)
    include(UseJava)

    # SCAPIX_JNI_PLATFORM_CLASSES
    # For JDK, this is the path to the jmods directory.
    # For JDK-8, this is the path to the "lib/ct.sym" file.
    #
    # Use the JDK found by FindJNI (the one whose headers and JVM we build against).
    # Its include directory is always "<jdk>/include", even when it is found via symlinks
    # (e.g. /usr/lib/jvm/default-java), unlike "javac" which is often a symlink or a launcher stub
    # (/usr/bin/javac on Linux and macOS, "Common Files/Oracle/Java/javapath" on Windows).
    # The JDK version is read from "<jdk>/release", which belongs to the same JDK.

    cmake_path(GET JAVA_INCLUDE_PATH PARENT_PATH jdk_home)

    set(jdk_version "")
    if(EXISTS "${jdk_home}/release")
        file(STRINGS "${jdk_home}/release" jdk_version REGEX "^JAVA_VERSION=" LIMIT_COUNT 1)
        string(REGEX REPLACE "^JAVA_VERSION=\"?([^\"]*)\"?$" "\\1" jdk_version "${jdk_version}")
    endif()

    if(jdk_version MATCHES "^1\\.")
        set(platform_classes "${jdk_home}/lib/ct.sym")
        set(platform_classes_type FILEPATH)
    else()
        set(platform_classes "${jdk_home}/jmods")
        set(platform_classes_type PATH)
    endif()

    # Replace a cached value that does not exist (e.g. computed by an earlier version of this file),
    # but never override a valid user-provided path.

    if(NOT EXISTS "${SCAPIX_JNI_PLATFORM_CLASSES}")
        set(SCAPIX_JNI_PLATFORM_CLASSES "${platform_classes}" CACHE ${platform_classes_type} "Path to Java platform classes" FORCE)
    endif()

endif()

message(STATUS "SCAPIX_JNI_PLATFORM_CLASSES: ${SCAPIX_JNI_PLATFORM_CLASSES}")

if(NOT EXISTS "${SCAPIX_JNI_PLATFORM_CLASSES}")
    set(hint "")
    if(NOT ANDROID AND SCAPIX_JNI_PLATFORM_CLASSES STREQUAL "${jdk_home}/jmods")
        string(REGEX MATCH "^[0-9]+" jdk_major "${jdk_version}")
        if(NOT jdk_major)
            set(jdk_major "<version>")
        endif()
        string(CONCAT hint
            "\nThis JDK (${jdk_home}) does not include the jmods directory. "
            "Some Linux distributions package jmods separately (e.g. 'openjdk-${jdk_major}-jmods'); install it if available. "
            "Some JDK 24+ builds do not ship jmods at all (JEP 493); use a JDK distribution that includes them. "
            "Alternatively, set SCAPIX_JNI_PLATFORM_CLASSES to an existing jmods directory.")
    endif()
    message(WARNING "SCAPIX_JNI_PLATFORM_CLASSES not found at: ${SCAPIX_JNI_PLATFORM_CLASSES}${hint}")
endif()

add_library(scapix_jni)
add_library(scapix::jni ALIAS scapix_jni)

target_include_directories(scapix_jni PUBLIC "source")
target_compile_features(scapix_jni PUBLIC cxx_std_20)
target_link_libraries(scapix_jni PUBLIC scapix::core)
set_property(TARGET scapix_jni PROPERTY POSITION_INDEPENDENT_CODE ON)

foreach(option IN ITEMS SCAPIX_JNI_CACHE_CLASS_LOADER SCAPIX_JNI_AUTO_ATTACH_THREAD SCAPIX_JNI_STANDARD_UTF8)
    message(STATUS "${option}: ${${option}}")

    if(${option})
        target_compile_definitions(scapix_jni PUBLIC ${option})
    endif()
endforeach()

if(WIN32)
    target_compile_definitions(scapix_jni PUBLIC SCAPIX_JNI_CLASS_PATH_SEPARATOR="\\x3b") # semicolon
else()
    target_compile_definitions(scapix_jni PUBLIC SCAPIX_JNI_CLASS_PATH_SEPARATOR=":")
endif()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "source/*")
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${sources})
target_sources(scapix_jni PRIVATE ${sources})

file(GLOB_RECURSE java_sources CONFIGURE_DEPENDS "source/*.java")
target_sources(scapix_jni INTERFACE ${java_sources})

if(NOT ANDROID)

    target_link_libraries(scapix_jni PUBLIC JNI::JVM)

    add_jar(scapix_jni_jar SOURCES ${java_sources} OUTPUT_NAME scapix_jni)
    add_dependencies(scapix_jni scapix_jni_jar)

    get_target_property(scapix_jni_jar_file scapix_jni_jar JAR_FILE)
    target_compile_definitions(scapix_jni PUBLIC SCAPIX_JNI_JAR_FILE="${scapix_jni_jar_file}")

endif()

function(scapix_jni_source_group)
    get_target_property(scapix_jni_dir scapix::jni SOURCE_DIR)
    get_target_property(sources scapix::jni INTERFACE_SOURCES)
    source_group(TREE "${scapix_jni_dir}/source" PREFIX "scapix-jni" FILES ${sources})
endfunction()

if(SCAPIX_JNI_BUILD_TESTS AND NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
