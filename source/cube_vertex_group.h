#ifndef CUBE_VERTEX_GROUP_H
#define CUBE_VERTEX_GROUP_H

#include "vertex_group.h"

// NOTE: tangent should be calculated based on normal and uv (texture coords)
const float CUBE2_VERTICES[] = {
    // coordinates         // normal              // texture coordinates    // tangent
    -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,    1.5f, -0.5f,              1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,    1.5f,  1.5f,              1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   -0.5f,  1.5f,              1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   -0.5f, -0.5f,              1.0f,  0.0f,  0.0f,   // top
     
    -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,    1.5f,  1.5f,              1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,    1.5f, -0.5f,              1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   -0.5f, -0.5f,              1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   -0.5f,  1.5f,              1.0f,  0.0f,  0.0f,   // bottom

    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,    1.5f, -0.5f,              0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   -0.5f, -0.5f,              0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,    1.5f,  1.5f,              0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   -0.5f,  1.5f,              0.0f,  0.0f,  1.0f,   //left

    -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,    1.5f, -0.5f,              1.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   -0.5f, -0.5f,              1.0f,  0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,    1.5f,  1.5f,              1.0f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   -0.5f,  1.5f,              1.0f,  0.0f, 0.0f,   // front

     0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,    1.5f, -0.5f,              0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   -0.5f, -0.5f,              0.0f,  0.0f, -1.0f,
     0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,    1.5f,  1.5f,              0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   -0.5f,  1.5f,              0.0f,  0.0f, -1.0f,   // right

    -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   -0.5f, -0.5f,             -1.0f,  0.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,    1.5f, -0.5f,             -1.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   -0.5f,  1.5f,             -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,    1.5f,  1.5f,             -1.0f,  0.0f, 0.0f,   // back
};
const unsigned int CUBE2_INDICES[] = {
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
const unsigned int CUBE2_ATTRIBS[] = {
    3,3,2,3
};

class CubeVertexGroup : public VertexGroup
{
  public:
    CubeVertexGroup()
        : VertexGroup(CUBE2_VERTICES, sizeof(CUBE2_VERTICES), // These are SIZE of the arrays data (bytes)
            CUBE2_INDICES, sizeof(CUBE2_INDICES),
            CUBE2_ATTRIBS, sizeof(CUBE2_ATTRIBS))
    {}
};

#endif // CUBE_VERTEX_GROUP_H