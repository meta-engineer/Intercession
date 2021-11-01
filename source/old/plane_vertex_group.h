
#ifndef PLANE_VERTEX_GROUP_H
#define PLANE_VERTEX_GROUP_H

#include "vertex_group.h"

float plane_vertices[] = {
    // coordinates          // color            // texture coordinates
     0.5f,  0.5f,  0.0f,   1.0f, 0.0f, 0.0f,    1.5f,  1.5f,
    -0.5f,  0.5f,  0.0f,   1.0f, 1.0f, 0.0f,   -0.5f,  1.5f,
    -0.5f, -0.5f,  0.0f,   0.0f, 1.0f, 1.0f,   -0.5f, -0.5f,
     0.5f, -0.5f,  0.0f,   0.0f, 0.0f, 1.0f,    1.5f, -0.5f
};
unsigned int plane_indices[] = {
    0,3,1,
    3,2,1
};
unsigned int plane_attribs[] = {
    3,3,2
};

class PlaneVertexGroup : public VertexGroup
{
  public:
    PlaneVertexGroup()
        : VertexGroup(plane_vertices, sizeof(plane_vertices), 
            plane_indices, sizeof(plane_indices), 
            plane_attribs, sizeof(plane_attribs))
    {}
};

#endif // PLANE_VERTEX_GROUP_H