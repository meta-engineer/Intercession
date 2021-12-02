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

Stencil functions:
glEnable(GL_STENCIL_TEST)
glStencilMask(0x00) aplies a bitmask to the value written into the buffer, 0x00 is used to disable writing
glStencilFunc(action, value, mask) use action to compare (mask & fragment's stencil value) with (mask & given value)
glStencilOp(sfail, dpfail, pass) output of stencilFunc is used to determine operation to use. one if it fails, one if it passed but depth fails, and one if both stencil and depth pass. operation changes (or not) the stencil buffer at the location of the frag

Shadows:
They are very complicated. No good algorithm, except raytracing...
Basic shadowmapping, build a FBO with a single depth component texture
render the scene to that texture (implicitly storing the depth)
when rendering the scene normally ingest the light view transform and that texture, read the .r property (depth values).
compare the fragment position's depth (in light view space) with the texture value at that projected position (the obscuring object) and compare.
if > its in shadow, else its the obscuring object itself, or some object closer (in the case of front face culling).

We can take a blur (PCF) around that and get soft shadows.

If we are using this depth component texture we can uniform it into the fs as a sampler2DShadow instead. This has builtin hardware bilinear filtering, when we sample it with texture().

Alternatively there is variance shadows. They are a little more confusing using some particular statistical math.
Follow Benny (set 1.25x spped) for a walkthrough. So there! yea... its, well there we go!
https://www.youtube.com/watch?v=mb7WuTDz5jw


Motion integration:
Euler method is FIRST-ORDER which means it is inaccurate for all but constant velocity (no acceleration)
What else is there though?

Verlet method:
We can cheaply get at least SECOND-ORDER accuracy to improve our simulation.
We need to add half the previous frame velocity and half the new frame velocity.
(averages out as linear interpolating between them)
so:
acceleration = (gravity/other forces)
position += velocity * (dt/2);
velocity += accelleration * dt;
position += velocity * (dt/2);

ForestRuth integration:
idk, its fourth order apperantly, and conserves momentum?

const float FR_Coefficient = 1.0/(2.0 - pow(2.0, 1.0/3.0));
const float FR_Compliment  =  1.0 - (2.0 * FR_Coefficient);

*do verlet with dt = dt*FR_Coefficient*
*do verlet with dt = dt*FR_Compliment*
*do verlet with dt = dt*FR_Coefficient (again)*


Animation:
GODLIKE Gdc talk
https://www.youtube.com/watch?v=LNidsMesxSE

Object controller first, and animations system shall "do no harm" to that!
VERY important for integrity and development modularity!

programatic animation will save time, make more consistent animations, make animation transitions automatically,
and create rich interpolated states multiplicatedly (see crouch walk)