#ifndef COLLECTIVE_ENTITY_H
#define COLLECTIVE_ENTITY_H

#include "entity.h"

// Eses instanced rendering to draw a group of similar Models/Meshes
class CollectiveEntity : public Entity
{
  public:
    // Inherit all constructors from Entity
    using Entity::Entity;

    // instance transforms should be set before invoking draw
    bool invoke_instanced_draw(ShaderManager& sm);
    bool invoke_instanced_draw(ShaderManager& sm, size_t amount);

    std::vector<glm::mat4> get_instance_transforms();
    // changing the transforms must be passed down to update Buffer Objects through setter
    bool set_instance_transforms(std::vector<glm::mat4> new_transforms);
    // SubBuffering...?
    // void update_instance_transforms(...);

  private:
    // implicit number of instances
    // TODO: To dynamically modify an instance I need to know the components of these matrices
    std::vector<glm::mat4> instance_transforms;

    // Instancing Buffer Object
    // Keep instance buffer data for all underlying meshes
    unsigned int IBO_ID;
};


bool CollectiveEntity::invoke_instanced_draw(ShaderManager& sm)
{
    return invoke_instanced_draw(sm, instance_transforms.size());
}
bool CollectiveEntity::invoke_instanced_draw(ShaderManager& sm, size_t amount)
{
    if (!graphics_model)
    {
        std::cerr << "Cannot invoke_instanced_draw on CollectiveEntity with no Model." << std::endl;
        return false;
    }
    if (amount > instance_transforms.size())
    {
        std::cerr << "Cannot invoke_instanced_draw an amount of instances greater than buffered instance data." << std::endl;
        return false;
    }

    sm.activate();
    sm.setMat4("model_to_world", get_model_transform());
    graphics_model->invoke_instanced_draw(sm, amount);

    return true;
}

std::vector<glm::mat4> CollectiveEntity::get_instance_transforms()
{
    return instance_transforms;
}

bool CollectiveEntity::set_instance_transforms(std::vector<glm::mat4> new_transforms)
{
    if (!graphics_model)
    {
        std::cerr << "Tried to invoke_draw on Entity with no Model." << std::endl;
        return false;
    }

    instance_transforms = new_transforms;
    
    // TODO: manage IBO_ID over more than one call
    glGenBuffers(1, &IBO_ID);
    glBindBuffer(GL_ARRAY_BUFFER, IBO_ID);
    glBufferData(GL_ARRAY_BUFFER, instance_transforms.size() * sizeof(glm::mat4), instance_transforms.data(), GL_STATIC_DRAW);

    // set attrib location 4->7 as transform matrix
    graphics_model->setup_all_instance_transform_attrib_arrays(4);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

// update_instance_transforms
//glBindBuffer(GL_ARRAY_BUFFER, IBO_ID);
//glBufferSubData(GL_ARRAY_BUFFER, offset, instance_transforms.size(), instance_transforms.data());

#endif //COLLECTIVE_ENTITY_H