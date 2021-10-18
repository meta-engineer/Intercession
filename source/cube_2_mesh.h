
#ifndef CUBE_2_MESH_H
#define CUBE_2_MESH_H

#include "mesh.h"

float cube2_verticies[] = {
    // coordinates         // normal             // color            // texture coordinates
    -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f, 0.0f,   1.5f,  1.5f,
    -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f, 0.0f,  -0.5f,  1.5f,
     0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f, 1.0f,  -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f, 1.0f,   1.5f, -0.5f,   // top

    -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f, 0.0f,   1.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f, 0.0f,   1.5f,  1.5f,
     0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f, 1.0f,  -0.5f,  1.5f,
     0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f, 1.0f,  -0.5f, -0.5f,   // bottom

    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f, 0.0f,   1.5f,  1.5f,
    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f, 0.0f,  -0.5f,  1.5f,
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f, 0.0f,   1.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f, 0.0f,   1.5f,  1.5f,   //left

    -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f, 0.0f,  -0.5f,  1.5f,
     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f, 1.0f,  -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f, 0.0f,   1.5f,  1.5f,
     0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f, 1.0f,  -0.5f,  1.5f,   // front

     0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f, 1.0f,  -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f, 1.0f,   1.5f, -0.5f,
     0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f, 1.0f,  -0.5f,  1.5f,
     0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f, 1.0f,  -0.5f, -0.5f,   // right

    -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f, 0.0f,   1.5f,  1.5f,
     0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f, 1.0f,   1.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f, 0.0f,   1.5f, -0.5f,
     0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f, 1.0f,  -0.5f, -0.5f,   // back
};
unsigned int cube2_indicies[] = {
    0,1,2,
    0,2,3,

    4,5,6,
    4,6,7,

    8,9,10,
    9,10,11,

    12,13,14,
    13,14,15,

    16,17,18,
    17,18,19,

    20,21,23,
    20,22,23
};
unsigned int cube2_attribs[] = {
    3,3,3,2
};

class Cube2Mesh : public Mesh
{
  public:
    Cube2Mesh()
        : Mesh(cube2_verticies, sizeof(cube2_verticies), // These are SIZE of the arrays data (bytes)
            cube2_indicies, sizeof(cube2_indicies),
            cube2_attribs, sizeof(cube2_attribs))
    {}
};

#endif // CUBE_2_MESH_H