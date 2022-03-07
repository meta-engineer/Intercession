#ifndef MODEL_BUILDER_H
#define MODEL_BUILDER_H

//#include "intercession_pch.h"
#include <string>

#include "model.h"
#include "vertex_group.h"

namespace pleep
{
    namespace model_builder
    {
        // TODO: move construction methods from Model to here

        // ***** Predefined Meshes *****

        // Build Model with standard quad buffer data
        // return shared pointer for ECS
        std::shared_ptr<Model> create_cube(std::string diffusePath = "", std::string specularPath = "", std::string normalPath = "", std::string displacePath = "");

        // Build Model with standard quad buffer data
        // the "create" functions should use some shared code
        std::unique_ptr<Model> create_quad(std::string diffusePath = "", std::string specularPath = "", std::string normalPath = "", std::string displacePath = "");

        // basic unit square for screen textures
        std::shared_ptr<VertexGroup> create_screen_plane();

        // ***** Resource Helper functions *****

        // generate texture from file, pass ownership (of gpu memory) to caller
        unsigned int load_gl_texture(std::string filename, const std::string& path = "", bool gammaCorrect = true);
        // generate cubemap texture from file, pass ownership (of gpu memory) to caller
        unsigned int load_gl_cubemap_texture(std::vector<std::string> filenames, bool gammaCorrect = true);

        // Use assimp to load a model from file
        //std::shared_ptr<> load_model(std::string path);
    }
}

#endif // MODEL_BUILDER_H