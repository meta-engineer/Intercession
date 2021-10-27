# External Libraries Configuration

## GLFW
GLFW is listed in .gitsubmodules and external/CMakeLists.txt should download it from github when invoked from the root CMakeLists.txt

## GLM
GLM is also listed in .gitsubmodules and external/CmakeLists.txt should download it from github when invoked.

## GLAD
GLAD is supposed to be generated through their web service:
https://glad.dav1d.de/
using options: C/C++, OpenGL, Core, gl(Version 4.6), "Generate a loader"
and unpacking the /include and /src directories to /external/glad

but it is small enough that it should be included in this repo

# stb_image
STB image is a single header library so we can just include it statically in our repo.

# assimp
Assimp is listed in .gitsubmodules. However LearnOpenGL reccommends using one of the pre-packaged downloads for it.
We'll use the submodule for convenience, but instability/incompadibility may arise.