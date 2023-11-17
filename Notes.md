# Notes on the tech stacks

## Git

check total lines of code:
`git ls-files | grep "^source/*" | xargs wc -l`

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

## Naming conventions
A good standard is hard to come by, consistent agreement is even harder.

List of things to name:
namespaces, class names, type/enum names, 
function names, method names, 
local variables, class variables, global variables, static variables, const variables

glfw and opengl use camelCase for functions (glGenbuffers)
However the std library uses lower_with_under (std::find_if)

google uses lower_with_under for variables
    and also a trailing underscore for class members
    and PascalCase for functions

iso uses lower_with_under for variables and functions

stroustrup uses lower_with_under for variables and functions
    but Pascal_With_Underscores for classes??

Here's what I'm thinking:

file_names:         lower_with_under
namespaces:         lower_with_under
function_names:     lower_with_under (to match std, and it works gramatically like a sentence)
ClassNames:         PascalCase
I_InterfaceClass:   PascalCase, I_ prefix. Abstract base class who's pointer is used for subclasses
A_AbstractClass:    PascalCase, A_ prefix. Abstract base class who's pointer is never used
T_TemplateType:     PascalCase, T_ prefix. Template specific type relative to this class/function.
class_method:       lower_with_under, private/protected methods prefixed with _

localVariables:     camelCase (looks like the uncapitalized type, EX: Model* model = ...
    this keeps each single variable "one word", can use an underscore sparingly for an attribute
    that would otherwise be a member of a struct. EX: bigChickenStore_id)
functionParameters: same as local variables (with Ref suffix if passed by reference?)
m_classVariables:   camelCase, m_ prefix (m for member)
g_globalVariables:  camelCase, g_ prefix
static::members:    camelCase, use explicit class namespace resolution
#MACRO_DEFINES:     UPPER_WITH_UNDER

Abbriviations & acronyms: treat as normal word (Not all caps)

Exceptions:
    gl buffer object ids can use all caps because they shouldn't be changed by *us*
but are not really const, only gl functions should be managing them.
    change-of-basis matricies can use x_to_y for readability when chaining transforms together

GLSL naming:
    attributes are camelCase with "a" prefix
    out variables are PascalCase (why? to denote them as the important "return" values for that shader)

    ill leave material names for the learnOpenGL shader structs as they are, because they'll likely be changing in future anyway.


## Application Architecture

| core/       | app/architecture components |
| logging/    | pleep logger based on spdlog |
| cosmos/     | ECS and world logic |
| rendering/  | render dynamo/relays |
| inputting/  | input parsing |
| networking/ | networking tools |

AppConfigBuilder:
 provide functions for AppGateway to create specific subclass of abstract ones
 (WindowApi -> GlfwWindowApi, RenderDynamo -> OpenGlRenderDynamo)

top level AppGateway class:
 creates WindowApi (provides generic equivalents of glfw calls)
 passes WindowApi to CosmosContext
 passes control to CosmosContext (main game loop)


Configuring cosmos data:
it may be usefull to provide script ingesting to build/modify scenes at runtime
LUA worked pretty good for cs488, and javidx9 can show how:
https://www.youtube.com/watch?v=4l5HdmPoynw

Otherwise using a serialized/readable data format for entities could be used to create/ingest

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

Textures:
Note that our load_gl_texture() function simply passes to glTexImage2D witht he data we read with stbi_load.
However we can create textures (as therefore use them in shaders) with any collection of structured 2D data.
It is probably easier to keep track of that data outside of the texture if we need to read it.
But for dynamic data (maybe deformable snow heightmap?) we can use glTexSubImage2D to update it.

Deferred Shading:
geometry pass: do a multi-render-pass to a 3 textures for the fragments: world position, normal, color & specular

lighting pass: ingest those 3 texture uniforms and calculate lighting for each screen fragment once.

Even with deferred lighting, we are checking all uniformed lights per fragment.
Ideally we'd have some space division in our LOD system so that we don't have every light in the world in uniforms.
IDK if its efficient to be reseting uniform information for each object.
perhaps with texture arrays to store shadow maps we can bind an unlimited amount.
There may be a space/time tradeoff whether we use cpu time to selectively uniform in light structs per object, or we store all lights for the whole scene.
Whatever upper bound of lights we uniform we can use...

SSAO:
requires VIEW space position (including depth) and normals. this is because the sampled points
around the frag need to be projected back into clip space.
(i would have thought using a world_to_view transform would fix that but idk it didn't work)
if this is required, as all the ssao articles use it (or maybe just optimal), this means
the standard deferred shading Geom pass doesn't work (in world space).
Ideally we don't convert that to view space, because all our lights are in world space when the lighting pass comes.
Maybe using world_to_view there works?

Most likely this means We'd have to do a second "ssao special" viewspace geom pass for these values.
Which means ssao is going to be more expensive than i thought.

I think I may skip it for now and we can re-try and performance test it when the proper Renderer and pipelines exist. Overall it is a subtle effect so its not that big a concern.
Something like Fresnel might be a larger impact (and probably easier).

Light Volumes: render a sphere at each point lights location with radius depending on attenuation.
the sphere will have no texure info, but
The vertex information will then selectively render to frags that the light COULD affect.
(note we need to use frontface culling so that the sphere is always visible, even from inside, and only ever affects a frag once)
Somehow we need to get a screen-space coordinate for the geometry textures (maybe gl_FragCoord?)
Then we render the fragment based on only that 1 lights info.
We need to use the right glBlendFunc so that colors from each rendered light are added togther.
This can only be possible with deferred rendering becuase we have already done a pass with the real objects and in the lighting step we can use any geometry to "stencil" the areas with light

Non-Photorealistic Rendering (NPR):
Different, less realistic shading techniques for interesting visual style.
Helps pull you out of the uncanny valley and coverup less detailed textures and animations.
https://www.youtube.com/watch?v=jlKNOirh66E

## Physics

Motion integration:
Euler method is FIRST-ORDER which means it is inaccurate for all but constant velocity (no acceleration)
What else is there though?

Improved Euler method:
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

*do imp. euler with dt = dt*FR_Coefficient*
*do imp. euler with dt = dt*FR_Compliment*
*do imp. euler with dt = dt*FR_Coefficient (again)*

Character controller setup:
Torso Hurtbox and leg springs
https://www.youtube.com/watch?v=qdskE8PJy6Q


## Animation
GODLIKE Gdc talk
https://www.youtube.com/watch?v=LNidsMesxSE

Object controller first, and animations system shall "do no harm" to that!
VERY important for integrity and development modularity!

programatic animation will save time, make more consistent animations, make animation transitions automatically,
and create rich interpolated states multiplicatedly (see crouch walk)

Maps and Materials:
diffuse maps are always needed as a base color

specular maps, according to learnopengl should be greyscale. I think this is so that only the colour of the light affects the highlight colour.
I guess specular light isn't affected by the surface, but only looks like it because it is mixed with diffuse?

normal maps define the normal vector as the texture colour vector. Is the GIMP normal map gereic filter good enough to get the benefit of normal mapping?
If the filter is based off bright/dark colours maybe you can boost contrast to improve the "normalling" filter?

QUATERNIONS:
http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/

## Networking
I forsee a network architecture being useful to implement the separate timelines required for the time travel mechanic.

This would have the added benefit of allowing multiplayer with little extra effort:
ASIO (and boost::ASIO) seems to be the weapon of choice, and javidx9 can show how:
https://www.youtube.com/watch?v=2hNdkYInj4g

There's also the extensive codersblock blog which uses raw sockets, but may provide further insight into setting up a packet system to provide good performance:
https://www.codersblock.org/blog/multiplayer-fps-part-1