
#ifndef CUBE_SMOOTH_VERTEX_GROUP_H
#define CUBE_SMOOTH_VERTEX_GROUP_H

#include "vertex_group.h"

const float CUBE_SMOOTH_VERTICES[] = {
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
const unsigned int CUBE_SMOOTH_INDICES[] = {
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
const unsigned int CUBE_SMOOTH_ATTRIBS[] = {
    3,3,2
};

class CubeSmoothVertexGroup : public VertexGroup
{
  public:
    CubeSmoothVertexGroup()
        : VertexGroup(CUBE_SMOOTH_VERTICES, sizeof(CUBE_SMOOTH_VERTICES) / sizeof(float),
            CUBE_SMOOTH_INDICES, sizeof(CUBE_SMOOTH_INDICES) / sizeof(unsigned int),
            CUBE_SMOOTH_ATTRIBS, sizeof(CUBE_SMOOTH_ATTRIBS) / sizeof(unsigned int))
    {}
};

#endif // CUBE_SMOOTH_VERTEX_GROUP_H