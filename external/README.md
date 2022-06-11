# External Libraries Configuration

## Header, Static and Dynamic Libraries
phew... what a topic. Way more confusing than one would assume.

There are a few methods to using libraries in cmake.
<hr>
For header-only files (like glm) you only need to target_include_directories() the location you'd like to start your #includes relative to.
The included headers will be built into your own source files and compile as usual
(Note precompiled headers are of consideration for these kind of external libraries)
<hr>
For libraries which need to compile you can use a, let's call it "immediate method" to include them.
The CMakeLists.txt included in these libraries (like glfw) will produce a library at build time. So from our project's CMakeList.txt tree we can add_subdirectory() to call the library's root CMakeLists.txt, let them build, and then add_library() (given we know the name used), this will build the library statically, directly into our binary. Then you target_include_directories() to the headers, like a header-only library.

This "immediate method" works simply, and has the advantage that all the configuration we do before calling our CMakeLists.txt (like debug/release mode) gets inherited by subdirectories and appropriately set. However it also requires recompiling the libraries every fresh build of our project unnecissarily (unless you modify their code), and it also inherits/invokes anything done inside the libraries CMakeLists.txt (although they may provide options()'s for some things)
<hr>

Preferrably, we precompile these libraries on their own, and then link them to our project afterward whenever we build.
First, build the libraries. If they use cmake, call their CMakeLists.txt, then invoke the --build. Note that you have to match the build config with the config used for our project.
Then we need to find the location of the cpp headers (maybe include/), the built .libs (maybe build/libs), and the built .dlls if the library was built dynamically (maybe build/bin). (Note that a .lib file could be a static library OR an import library for a .dll)

To link these artifacts, first target_link_directories() to the location of the .lib
Then target_link_libraries() with the name of the library (the filename before .lib).
Also target_include_directories() the location of the cpp headers as with all types of libraries.

Finally, if the library was a .dll we've linked the import library (.lib) but need to supply the dll. So far the only painless way to do this is to copy the .dll into your executable's directory after building. (perhaps with a cmake execute_command, or other script).

Note also that becuase the .dll is not built-in it must also be included when packaging/installing the software. So the .dll needs to be install()ed to bin
<hr>

Note all build use default "architecture" which seems to be 32 bit. If in future 64 bit is desired, compiled libs and Intercession executable must be both changed with the "cmake -A" option (and update their build scripts here).

## GLFW
GLFW is listed in .gitsubmodules and external/CMakeLists.txt should download it from github when invoked from the root CMakeLists.txt

For MSVC (a multi config build system), you can't supply build config at cmake-runtime, but have to supply at build time with --config Release/Debug.
It is a static lib (by default) in build/src/${CONFIGURATION}/glfw3.lib

## GLM
GLM is also listed in .gitsubmodules and external/CmakeLists.txt should download it from github when invoked.

Header only, so it is simply included.

## GLAD
GLAD is supposed to be generated through their web service:
https://glad.dav1d.de/
using options: C/C++, OpenGL, Core, gl(Version 4.6), "Generate a loader"
and unpacking the /include and /src directories to /external/glad

but it is small enough that it should be included in this repo

Has header and source, but is small, so to simplify linking we'll add its source to our project CMakeLists.txt and include the headers in external/CMakeLists.txt

# stb_image
STB image is a single header library so we can just include it statically in our repo.

Header only, so it is simply included.

# assimp
Assimp is listed in .gitsubmodules. However LearnOpenGL reccommends using one of the pre-packaged downloads for it.
We'll use the submodule for convenience, but instability/incompadibility may arise.

For MSVC (a multi config build system), you can't supply build config at cmake-runtime, but have to supply at build time with --config Release/Debug.

It is a dynamic lib (by default) in build/code/${CONFIGURATION}/assimp-vc141-mt(d).dll which needs to be moved to our executable.
Note that it post-pends d when built in debug config.
Its import lib is in build/lib/${CONFIGURATION}/assimp-vc141-mt(d).lib
use target_link_libraries options "debug <name>" and "optimized <name>" to specify the version with/without postfixes.

NOTE: we can compile it as a static lib by DISABLING BUILD_SHARED_LIBS and ENABLING ASSIMP_BUILD_ZLIB
then the aforementioned assimp-vc141-mt(d).lib becomes the whole static library to be linked.
We now have to link build/contrib/zlib/${Configuration}/zlibstatic(d).lib

# spdlog
spdlog seems to be compile optional, so we'll use header-only for ease.

Simply included headers.

NOTE: spdlog invludes minwindef.h which defines APIENTRY.
glfw also defined APIENTRY. winwindef.h does not have a redefine guard.
So in pleep_logger.h I manually #undef APIENTRY to fix it.
Hopefully this causes no long term bugs...

# asio
asio is the api used for networking. The library is downloaded directly from the website:
https://think-async.com/Asio/
and unzipped into external/asio/

It is header only so it can be directly included.