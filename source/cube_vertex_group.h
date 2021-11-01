#ifndef CUBE_VERTEX_GROUP_H
#define CUBE_VERTEX_GROUP_H

#include "vertex_group.h"

float cube2_vertices[] = {
    // coordinates         // normal              // texture coordinates
    -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,    1.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,    0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,    0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,    1.0f,  0.0f,   // top

    -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,    1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,    1.0f,  1.0f,
     0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,    0.0f,  1.0f,
     0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,    0.0f,  0.0f,   // bottom

    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,    1.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,    0.0f,  1.0f,
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,    1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,    0.0f,  0.0f,   //left

    -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,    1.0f,  1.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,    0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,    1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,    0.0f,  0.0f,   // front

     0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,    1.0f,  1.0f,
     0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,    0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,    1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,    0.0f,  0.0f,   // right

    -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,    1.0f,  1.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,    0.0f,  1.0f,
    -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,    1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,    0.0f,  0.0f,   // back
};
unsigned int cube2_indices[] = {
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
    3,3,2
};

class CubeVertexGroup : public VertexGroup
{
  public:
    CubeVertexGroup()
        : VertexGroup(cube2_vertices, sizeof(cube2_vertices), // These are SIZE of the arrays data (bytes)
            cube2_indices, sizeof(cube2_indices),
            cube2_attribs, sizeof(cube2_attribs))
    {}
};

#endif // CUBE_VERTEX_GROUP_H