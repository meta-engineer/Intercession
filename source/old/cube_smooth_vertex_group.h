
#ifndef CUBE_SMOOTH_VERTEX_GROUP_H
#define CUBE_SMOOTH_VERTEX_GROUP_H

#include "vertex_group.h"

float cube_smooth_vertices[] = {
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
unsigned int cube_smooth_indices[] = {
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
unsigned int cube_smooth_attribs[] = {
    3,3,2
};

class CubeSmoothVertexGroup : public VertexGroup
{
  public:
    CubeSmoothVertexGroup()
        : VertexGroup(cube_smooth_vertices, sizeof(cube_smooth_vertices), // These are SIZE of the arrays data (bytes)
            cube_smooth_indices, sizeof(cube_smooth_indices),
            cube_smooth_attribs, sizeof(cube_smooth_attribs))
    {}
};

#endif // CUBE_SMOOTH_VERTEX_GROUP_H