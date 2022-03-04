#ifndef MODEL_BUILDER_H
#define MODEL_BUILDER_H

//#include "intercession_pch.h"
#include "model.h"
#include "vertex_group.h"

namespace pleep
{
    namespace model_builder
    {
        // TODO: move construction methods from Model to here

        // Build Model with standard quad buffer data
        // return shared pointer for ECS
        std::shared_ptr<Model> create_cube(std::string diffusePath = "", std::string specularPath = "", std::string normalPath = "", std::string displacePath = "");

        
        // Build Model with standard quad buffer data
        // the "create" functions should use some shared code
        std::unique_ptr<Model> create_quad(std::string diffusePath = "", std::string specularPath = "", std::string normalPath = "", std::string displacePath = "");

        // basic unit square for screen textures
        std::shared_ptr<VertexGroup> create_screen_plane();
    }
}

#endif // MODEL_BUILDER_H