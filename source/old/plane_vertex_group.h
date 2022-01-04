
#ifndef PLANE_VERTEX_GROUP_H
#define PLANE_VERTEX_GROUP_H

#include "vertex_group.h"

const float PLANE_VERTICES[] = {
    // coordinates          // color            // texture coordinates
     0.5f,  0.5f,  0.0f,   1.0f, 0.0f, 0.0f,    1.5f,  1.5f,
    -0.5f,  0.5f,  0.0f,   1.0f, 1.0f, 0.0f,   -0.5f,  1.5f,
    -0.5f, -0.5f,  0.0f,   0.0f, 1.0f, 1.0f,   -0.5f, -0.5f,
     0.5f, -0.5f,  0.0f,   0.0f, 0.0f, 1.0f,    1.5f, -0.5f
};
const unsigned int PLANE_INDICES[] = {
    0,3,1,
    3,2,1
};
const unsigned int PLANE_ATTRIBS[] = {
    3,3,2
};

class PlaneVertexGroup : public VertexGroup
{
  public:
    PlaneVertexGroup()
        : VertexGroup(PLANE_VERTICES, sizeof(PLANE_VERTICES) / sizeof(float), 
            PLANE_INDICES, sizeof(PLANE_INDICES) / sizeof(unsigned int), 
            PLANE_ATTRIBS, sizeof(PLANE_ATTRIBS) / sizeof(unsigned int))
    {}
};

#endif // PLANE_VERTEX_GROUP_H