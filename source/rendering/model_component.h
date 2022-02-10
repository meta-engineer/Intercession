#ifndef MODEL_COMPONENT_H
#define MODEL_COMPONENT_H

//#include "intercession_pch.h"
#include <vector>
#include <string>
#include <map>
#include <cassert>

#include "model.h"


namespace pleep
{
    // contains all meshes related to a single entity
    // given ecs is more functional there will need to be accompanying
    // methods for building/modifying mesh data
    struct ModelComponent
    {
        // the cherno uses components with shared pointers
        //   so i'll assume that it is reasonably performant, though maybe not ideal
        // for now we'll reuse our test class
        // TODO: make his more data-oriented
        std::shared_ptr<Model> model;

        // model builder should maintain these while constructing all meshes
        // It is unlikely we'll me changing a model so significantly after construction
        //std::string directory;
        //std::vector<Texture> texturesLoaded;

        // ideally armatures would be in a completely seperate component
        // but the bone data may be tied to the specific model

		ModelComponent() = default;
		ModelComponent(const ModelComponent&) = default;
        ModelComponent(std::shared_ptr<Model> initModel)
            : model(initModel)
        {}
    };

    // we may want multiple entities to share mesh data.
    // This could be accomplished by building them into instances and
    //   merging them to one instanced draw call
    // Alternatively we could have them use a flyweight model
    // Where each user would reference them same (const) data (VAO, EBO, EBO)
    // then they would differ in uniforms set by other components (transformcomponent)
}

#endif // MODEL_COMPONENT_H