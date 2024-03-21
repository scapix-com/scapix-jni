# Scapix JNI

🇺🇦 If you like this project, please consider donating to one of the funds helping victims of russian aggression against Ukraine: [ukraine.ua](https://war.ukraine.ua/donate/)

Modern C++20 wrapper for JNI (Java Native Interface):
- type-safe APIs
- automatic resource management
- ZERO runtime overhead compared to manually written JNI code
- automatic C++/Java type conversion for many standard types (std::string, std::vector, etc.)
- automatic C++/Java exception tunneling
- comes with pre-generated C++ headers for all JDK and Android Java APIs
- automatically generate C++ headers for any Java code, including your own

```cpp
// generated headers for all JDK/Android classes
#include <scapix/java_api/java/lang/System.h>
#include <scapix/java_api/java/util/Locale.h>
#include <scapix/java_api/java/text/DateFormatSymbols.h>

using namespace scapix::java_api;

void test()
{
    // Automatic convertion between common C++ and Java types (std::string, std::vector, std::map, etc)

    std::string version = java::lang::System::getProperty("java.version");
    std::vector<std::string> languages = java::util::Locale::getISOLanguages();
    std::vector<std::vector<std::string>> zone_strings = java::text::DateFormatSymbols::getInstance()->getZoneStrings();
    std::map<std::string, std::string> properties = java::lang::System::getProperties();
}
```

[Documentation](https://www.scapix.com/java_link)\
[Example](https://github.com/scapix-com/example2)

## License

Please carefully read [license agreement](LICENSE.txt).

**In short:**
If you comply with [license agreement](LICENSE.txt), you may use [Scapix JNI](https://www.scapix.com) free of charge to build commercial and/or open source applications.
You may NOT modify and/or redistribute the [Scapix JNI](https://www.scapix.com) product itself.
