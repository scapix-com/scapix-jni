/*
	scapix/jni/java_api/init.h

	Copyright (c) 2019-2024 Boris Rasin (boris@scapix.com)
*/

#ifndef SCAPIX_JNI_JAVA_API_INIT_H
#define SCAPIX_JNI_JAVA_API_INIT_H

#include <scapix/jni/object_base.h>
#include <scapix/jni/ref.h>
#include <scapix/jni/array.h>
#include <scapix/jni/convert.h>
#include <scapix/jni/object_traits.h>

namespace scapix::java_api {

// workaround for conflict between namespaces "sun" and "com::sun"

namespace sun {}
namespace com::sun { namespace sun = ::scapix::java_api::sun; }

// workaround for conflict between namespaces "java" and "com::sun::java"

namespace java {}

namespace com::sun::accessibility { namespace java = ::scapix::java_api::java; }
namespace com::sun::awt { namespace java = ::scapix::java_api::java; }
namespace com::sun::beans { namespace java = ::scapix::java_api::java; }
namespace com::sun::corba { namespace java = ::scapix::java_api::java; }
namespace com::sun::imageio { namespace java = ::scapix::java_api::java; }
namespace com::sun::jarsigner { namespace java = ::scapix::java_api::java; }
namespace com::sun::java { namespace java = ::scapix::java_api::java; }
namespace com::sun::javadoc { namespace java = ::scapix::java_api::java; }
namespace com::sun::java_cup { namespace java = ::scapix::java_api::java; }
namespace com::sun::jdi { namespace java = ::scapix::java_api::java; }
namespace com::sun::jmx { namespace java = ::scapix::java_api::java; }
namespace com::sun::jndi { namespace java = ::scapix::java_api::java; }
namespace com::sun::management { namespace java = ::scapix::java_api::java; }
namespace com::sun::media { namespace java = ::scapix::java_api::java; }
namespace com::sun::naming { namespace java = ::scapix::java_api::java; }
namespace com::sun::net { namespace java = ::scapix::java_api::java; }
namespace com::sun::nio { namespace java = ::scapix::java_api::java; }
namespace com::sun::org { namespace java = ::scapix::java_api::java; }
namespace com::sun::rmi { namespace java = ::scapix::java_api::java; }
namespace com::sun::rowset { namespace java = ::scapix::java_api::java; }
namespace com::sun::security { namespace java = ::scapix::java_api::java; }
namespace com::sun::source { namespace java = ::scapix::java_api::java; }
namespace com::sun::swing { namespace java = ::scapix::java_api::java; }
namespace com::sun::tools { namespace java = ::scapix::java_api::java; }
namespace com::sun::tracing { namespace java = ::scapix::java_api::java; }

// workaround for conflict between namespaces "org" and "com::sun::org"

namespace org {}

namespace com::sun::accessibility { namespace org = ::scapix::java_api::org; }
namespace com::sun::awt { namespace org = ::scapix::java_api::org; }
namespace com::sun::beans { namespace org = ::scapix::java_api::org; }
namespace com::sun::corba { namespace org = ::scapix::java_api::org; }
namespace com::sun::imageio { namespace org = ::scapix::java_api::org; }
namespace com::sun::jarsigner { namespace org = ::scapix::java_api::org; }
namespace com::sun::java { namespace org = ::scapix::java_api::org; }
namespace com::sun::javadoc { namespace org = ::scapix::java_api::org; }
namespace com::sun::java_cup { namespace org = ::scapix::java_api::org; }
namespace com::sun::jdi { namespace org = ::scapix::java_api::org; }
namespace com::sun::jmx { namespace org = ::scapix::java_api::org; }
namespace com::sun::jndi { namespace org = ::scapix::java_api::org; }
namespace com::sun::management { namespace org = ::scapix::java_api::org; }
namespace com::sun::media { namespace org = ::scapix::java_api::org; }
namespace com::sun::naming { namespace org = ::scapix::java_api::org; }
namespace com::sun::net { namespace org = ::scapix::java_api::org; }
namespace com::sun::nio { namespace org = ::scapix::java_api::org; }
namespace com::sun::org { namespace org = ::scapix::java_api::org; }
namespace com::sun::rmi { namespace org = ::scapix::java_api::org; }
namespace com::sun::rowset { namespace org = ::scapix::java_api::org; }
namespace com::sun::security { namespace org = ::scapix::java_api::org; }
namespace com::sun::source { namespace org = ::scapix::java_api::org; }
namespace com::sun::swing { namespace org = ::scapix::java_api::org; }
namespace com::sun::tools { namespace org = ::scapix::java_api::org; }
namespace com::sun::tracing { namespace org = ::scapix::java_api::org; }

} // namespace scapix::java_api

#endif // SCAPIX_JNI_JAVA_API_INIT_H
