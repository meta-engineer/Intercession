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

    4,6,5,
    4,7,6,

    8,10,9,
    9,10,11,

    12,14,13,
    13,14,15,

    16,18,17,
    17,18,19,

    20,21,23,
    20,23,22
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