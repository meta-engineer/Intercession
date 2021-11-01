
#ifndef SKYBOX_VERTEX_GROUP_H
#define SKYBOX_VERTEX_GROUP_H

#include "vertex_group.h"

float skybox_vertices[] = {
    // coordinates         
    -0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f, -0.5f,  // top

    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f, -0.5f,  // bottom

    -0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,  //left

    -0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,  // front

     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f, -0.5f, -0.5f,  // right

    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f   // back
};
unsigned int skybox_indices[] = {
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
unsigned int skybox_attribs[] = {
    3
};

class SkyboxVertexGroup : public VertexGroup
{
  public:
    SkyboxVertexGroup()
        : VertexGroup(skybox_vertices, sizeof(skybox_vertices), // These are SIZE of the arrays data (bytes)
            skybox_indices, sizeof(skybox_indices),
            skybox_attribs, sizeof(skybox_attribs))
    {}
};

#endif // SKYBOX_VERTEX_GROUP_H