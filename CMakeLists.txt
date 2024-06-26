cmake_minimum_required(VERSION 3.20...3.27)

project(scapix-jni CXX)

option(SCAPIX_JNI_CACHE_CLASS_LOADER "" ON)
option(SCAPIX_JNI_AUTO_ATTACH_THREAD "" ON)

message(STATUS "SCAPIX_JNI_CACHE_CLASS_LOADER: ${SCAPIX_JNI_CACHE_CLASS_LOADER}")
message(STATUS "SCAPIX_JNI_AUTO_ATTACH_THREAD: ${SCAPIX_JNI_AUTO_ATTACH_THREAD}")

add_library(scapix_jni)
add_library(scapix::jni ALIAS scapix_jni)

target_include_directories(scapix_jni PUBLIC source)
target_compile_features(scapix_jni PUBLIC cxx_std_20)
set_property(TARGET scapix_jni PROPERTY POSITION_INDEPENDENT_CODE ON)

if(SCAPIX_JNI_CACHE_CLASS_LOADER)
    target_compile_definitions(scapix_jni PUBLIC SCAPIX_JNI_CACHE_CLASS_LOADER)
endif()

if(SCAPIX_JNI_AUTO_ATTACH_THREAD)
    target_compile_definitions(scapix_jni PUBLIC SCAPIX_JNI_AUTO_ATTACH_THREAD)
endif()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS source/*)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/source" PREFIX "source" FILES ${sources})
target_sources(scapix_jni PRIVATE ${sources})

file(GLOB_RECURSE java_sources CONFIGURE_DEPENDS source/*.java)
target_sources(scapix_jni INTERFACE ${java_sources})

find_package(ScapixCore REQUIRED)
target_link_libraries(scapix_jni PUBLIC scapix::core)

if(WIN32)
    target_compile_definitions(scapix_jni PUBLIC SCAPIX_JNI_CLASS_PATH_SEPARATOR="\\x3b") # semicolon
else()
    target_compile_definitions(scapix_jni PUBLIC SCAPIX_JNI_CLASS_PATH_SEPARATOR=":")
endif()

if(NOT ANDROID)
    # Save CMAKE_FIND_FRAMEWORK
    if(DEFINED CMAKE_FIND_FRAMEWORK)
        set(SCAPIX_CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK})
    else()
        unset(SCAPIX_CMAKE_FIND_FRAMEWORK)
    endif()

    set(CMAKE_FIND_FRAMEWORK LAST)
    find_package(JNI REQUIRED)

    # Restore CMAKE_FIND_FRAMEWORK
    if(DEFINED SCAPIX_CMAKE_FIND_FRAMEWORK)
        set(CMAKE_FIND_FRAMEWORK ${SCAPIX_CMAKE_FIND_FRAMEWORK})
        unset(SCAPIX_CMAKE_FIND_FRAMEWORK)
    else()
        unset(CMAKE_FIND_FRAMEWORK)
    endif()

    target_include_directories(scapix_jni PUBLIC ${JNI_INCLUDE_DIRS})
    target_link_libraries(scapix_jni PUBLIC ${JNI_LIBRARIES})

    find_package(Java REQUIRED COMPONENTS Development)
    include(UseJava)

#   file(GLOB_RECURSE java_sources CONFIGURE_DEPENDS source/*.java)
    add_jar(scapix_jni_jar SOURCES ${java_sources} OUTPUT_NAME scapix_jni)

    add_dependencies(scapix_jni scapix_jni_jar)

    get_target_property(scapix_jni_jar_file scapix_jni_jar JAR_FILE)
    target_compile_definitions(scapix_jni PUBLIC SCAPIX_JNI_JAR_FILE="${scapix_jni_jar_file}")
endif()

function(scapix_jni_source_group)
    get_target_property(scapix_jni_dir scapix::jni SOURCE_DIR)
    get_target_property(sources scapix::jni INTERFACE_SOURCES)
    source_group(TREE ${scapix_jni_dir}/source PREFIX "scapix-jni" FILES ${sources})
endfunction()
