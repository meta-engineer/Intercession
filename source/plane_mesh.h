
#ifndef PLACE_MESH_H
#define PLACE_MESH_H

#include "mesh.h"

float plane_verticies[] = {
    // coordinates          // color            // texture coordinates
     0.5f,  0.45f,  0.0f,   1.0f, 0.0f, 0.0f,    1.5f,  1.5f,
    -0.5f,  0.45f,  0.0f,   1.0f, 1.0f, 0.0f,   -0.5f,  1.5f,
    -0.5f, -0.45f,  0.0f,   0.0f, 1.0f, 1.0f,   -0.5f, -0.5f,
     0.5f, -0.45f,  0.0f,   0.0f, 0.0f, 1.0f,    1.5f, -0.5f
};
unsigned int plane_indicies[] = {
    0,3,1,
    3,2,1
};
unsigned int plane_attribs[] = {
    3,3,2
};

class PlaneMesh : public Mesh
{
  public:
    PlaneMesh()
        : Mesh(plane_verticies, sizeof(plane_verticies), 
            plane_indicies, sizeof(plane_indicies), 
            plane_attribs, sizeof(plane_attribs))
    {}
};

#endif // PLACE_MESH_H