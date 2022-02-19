#ifndef MODEL_BUILDER_H
#define MODEL_BUILDER_H

//#include "intercession_pch.h"
#include "model.h"

namespace pleep
{
    namespace model_builder
    {
        // TODO: move construction methods from Model to here

        // Build Model with standard quad buffer data
        // return shared pointer for ECS
        std::shared_ptr<Model> create_cube(std::string diffusePath = "", std::string specularPath = "", std::string normalPath = "");
    }
}

#endif // MODEL_BUILDER_H