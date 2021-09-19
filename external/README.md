# External Libraries Configuration

## GLFW
GLFW is listed in .gitsubmodules and external/CMakeLists.txt should download it from github when invoked from the root CMakeLists.txt

## GLAD
GLAD is supposed to be generated through their web service:
https://glad.dav1d.de/
using options: C/C++, OpenGL, Core, gl(Version 4.6), "Generate a loader"
and unpacking the /include and /src directories to /external/glad

but it is small enough that it should be included in this repo