# Using Cmake
$ cmake . -B ./build
$ cmake --build ./build

ctt tutorial
https://www.youtube.com/watch?v=nlKcXPUJGwA&list=PLalVdRk2RC6o5GHu618ARWh0VO0bFlif4&index=1

Use namespaces within your libraries so source has to address the explicit namespace for methods.

add_subdirectory() calls the CMakeLists for that directory
add_library() creates a library for the passed source (like add_executable)
target_link_libraries() links to a library created elsewhere by add_library()
target_include_directories() makes headers accessable (and other files) by the root source.


# using opengl/glfw

https://learnopengl.com/