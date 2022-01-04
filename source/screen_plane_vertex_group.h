#ifndef SCREEN_PLANE_VERTEX_GROUP_H
#define SCREEN_PLANE_VERTEX_GROUP_H

#include "vertex_group.h"

const float SCREEN_PLANE_VERTICES[] = {
    // coordinates       // texture coordinates
    -1.0f,  1.0f,  0.0f,   0.0f,  1.0f,
     1.0f,  1.0f,  0.0f,   1.0f,  1.0f,
    -1.0f, -1.0f,  0.0f,   0.0f, 0.0f,
     1.0f, -1.0f,  0.0f,   1.0f, 0.0f
};
const unsigned int SCREEN_PLANE_INDICES[] = {
    0,2,1,
    1,2,3
};
const unsigned int SCREEN_PLANE_ATTRIBS[] = {
    3,2
};

class ScreenPlaneVertexGroup : public VertexGroup
{
  public:
    ScreenPlaneVertexGroup()
        : VertexGroup(SCREEN_PLANE_VERTICES, sizeof(SCREEN_PLANE_VERTICES) / sizeof(float), 
            SCREEN_PLANE_INDICES, sizeof(SCREEN_PLANE_INDICES) / sizeof(unsigned int), 
            SCREEN_PLANE_ATTRIBS, sizeof(SCREEN_PLANE_ATTRIBS) / sizeof(unsigned int))
    {}
};

#endif // SCREEN_PLANE_VERTEX_GROUP_H