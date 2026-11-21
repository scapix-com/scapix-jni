# cmodule 3.0.0: builds dependencies from source as part of the build.
# https://www.scapix.com/cmodule
#
# Load via CMAKE_PROJECT_TOP_LEVEL_INCLUDES (e.g. in CMakePresets.json), or:
#   cmake -B build -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=cmake/cmodule.cmake
#
# Upgrade to the latest release:
#   curl -L -o cmake/cmodule.cmake https://github.com/scapix-com/cmodule/releases/latest/download/cmodule.cmake

include(FetchContent)
FetchContent_Declare(
  cmodule
  URL https://github.com/scapix-com/cmodule/releases/download/v3.0.0/cmodule-3.0.0.tar.gz
  URL_HASH SHA256=4e083d07848c4387bbdaab0c079d52ea6f18c4daa83c93d9d7b447f4ba5c8623
)
FetchContent_MakeAvailable(cmodule)
