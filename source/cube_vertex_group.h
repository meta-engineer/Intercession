
#ifndef CUBE_VERTEX_GROUP_H
#define CUBE_VERTEX_GROUP_H

#include "vertex_group.h"

float cube_verticies[] = {
    // coordinates         // normal             // texture coordinates
    -0.5f,  0.5f, -0.5f,  -1.0f,  1.0f, -1.0f,    1.5f,  1.5f,
    -0.5f,  0.5f,  0.5f,  -1.0f,  1.0f,  1.0f,   -0.5f,  1.5f,
     0.5f,  0.5f,  0.5f,   1.0f,  1.0f,  1.0f,   -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,   1.0f,  1.0f, -1.0f,    1.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,  -1.0f, -1.0f, -1.0f,    1.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,  -1.0f, -1.0f,  1.0f,    1.5f,  1.5f,
     0.5f, -0.5f,  0.5f,   1.0f, -1.0f,  1.0f,   -0.5f,  1.5f,
     0.5f, -0.5f, -0.5f,   1.0f, -1.0f, -1.0f,   -0.5f, -0.5f
};
unsigned int cube_indicies[] = {
    0,1,2,
    0,2,3,
    0,1,4,
    1,4,5,
    1,2,5,
    2,5,6,
    2,3,6,
    3,6,7,
    3,0,7,
    0,7,4,
    4,5,6,
    4,7,6
};
unsigned int cube_attribs[] = {
    3,3,2
};

class CubeVertexGroup : public VertexGroup
{
  public:
    CubeVertexGroup()
        : VertexGroup(cube_verticies, sizeof(cube_verticies), // These are SIZE of the arrays data (bytes)
            cube_indicies, sizeof(cube_indicies),
            cube_attribs, sizeof(cube_attribs))
    {}
};

#endif // CUBE_VERTEX_GROUP_H