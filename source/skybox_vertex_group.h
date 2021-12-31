#ifndef SKYBOX_VERTEX_GROUP_H
#define SKYBOX_VERTEX_GROUP_H

#include "vertex_group.h"

const float SKYBOX_VERTICES[] = {
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
const unsigned int SKYBOX_INDICES[] = {
    0,2,1,
    0,3,2,

    4,5,6,
    4,6,7,

    8,9,10,
    9,11,10,

    12,13,14,
    13,15,14,

    16,17,18,
    17,19,18,

    20,23,21,
    20,22,23
};
const unsigned int SKYBOX_ATTRIBS[] = {
    3
};

// Skybox is a cube (like CubeVertexGroup), but winding order for faces is inward
class SkyboxVertexGroup : public VertexGroup
{
  public:
    SkyboxVertexGroup()
        : VertexGroup(SKYBOX_VERTICES, sizeof(SKYBOX_VERTICES), // These are SIZE of the arrays data (bytes)
            SKYBOX_INDICES, sizeof(SKYBOX_INDICES),
            SKYBOX_ATTRIBS, sizeof(SKYBOX_ATTRIBS))
    {}
};

#endif // SKYBOX_VERTEX_GROUP_H