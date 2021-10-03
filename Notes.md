# Notes on the tech stacks

## CTT tutorial
https://www.youtube.com/watch?v=nlKcXPUJGwA&list=PLalVdRk2RC6o5GHu618ARWh0VO0bFlif4&index=1

## Using Cmake
$ cmake . -B build
$ cmake --build build

also for realease config
$cmake --build build --config Release

Use namespaces within your libraries so source has to address the explicit namespace for methods.

add_subdirectory() calls the CMakeLists for that directory
add_library() creates a library for the passed source (like add_executable)
target_link_libraries() links to a library created elsewhere by add_library()
target_include_directories() makes headers accessable (and other files) by the root source.

## Using git submodules
$ git submodule add GIT_URL PATH
downloads and adds the submodule to .gitmodules

now the user can run
$ git submodule update --init --recursive
to get the external libraries

this can also be invoked from cmake.
see : external/CMakeLists.txt

## Using cmake install
install requires the Release version has been built:
    ON LINUX it would be specifying -DCMAKE_BUILD_TYPE=Release (case sensitive) in the cmake configure step
    ON WINDOWS (for vs) use $cmake --build build --config Release

then...
$cmake --install ./build
Loads the binary into Program Files
(PATH is not updated for default cmd use unlike linux, so add PROJECT_NAME/bin to PATH env)

## Using CPack (more based than cmake install)
CPack cannot ingest a working directory (not so based) so you must
$cd build; cpack

CPack ALSO requires Release verison to be built!
(and it doesn't give you a helpful hint for that, it just throws an error)

## Hiding the Console
This likely will be platform dependant.
Here's two options i've seen:

(works for mvsc/Visual Studio compiler)
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS"
)

(maybe only works for other compilers)
target_link_options(${PROJECT_NAME}
   PRIVATE "-mwindows"
)

## Using opengl/glfw

https://learnopengl.com/

Some useful definitions.

1. Shader: a "small" program for one step of the graphics pipeline
2. Vertex Shader: ingests a single vertex, transform 3D to 2D, process vertex attributes
3. Primitive assembly: ingests verticies that form a chosen primitive (point/line/triangle) and groups them accordingly
4. geometry shader: ingests a primitive (and all verticies within), generates new shapes and verticies to form new/other primitives
5. rasterization: ingests a primitive, maps to the corresponding pixels creating a fragment.
    *Fragment: a single mapped pixel and the primitive data required to determine its rendering 
6. fragment shader: ingests a fragment, outputs the final color of the pixel
7. alpha test/blending: ingests the final object of pixels, checks depth of fragments/ alpha of graments and blends/discards the color values into the final buffer.
8. shader program: linked version of multiple shaders combined. inputs and outputs of consecutive shaders must match.
9. uniform: pass data from app (cpu) to shaders (gpu)