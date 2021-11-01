
#ifndef SCREEN_PLANE_VERTEX_GROUP_H
#define SCREEN_PLANE_VERTEX_GROUP_H

#include "vertex_group.h"

float screen_plane_vertices[] = {
    // coordinates       // texture coordinates
    -1.0f,  1.0f,  0.0f,   0.0f,  1.0f,
     1.0f,  1.0f,  0.0f,   1.0f,  1.0f,
    -1.0f, -1.0f,  0.0f,   0.0f, 0.0f,
     1.0f, -1.0f,  0.0f,   1.0f, 0.0f
};
unsigned int screen_plane_indices[] = {
    0,2,1,
    1,2,3
};
unsigned int screen_plane_attribs[] = {
    3,2
};

class ScreenPlaneVertexGroup : public VertexGroup
{
  public:
    ScreenPlaneVertexGroup()
        : VertexGroup(screen_plane_vertices, sizeof(screen_plane_vertices), 
            screen_plane_indices, sizeof(screen_plane_indices), 
            screen_plane_attribs, sizeof(screen_plane_attribs))
    {}
};

#endif // SCREEN_PLANE_VERTEX_GROUP_H