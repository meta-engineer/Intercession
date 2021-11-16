
#ifndef ENTITY_H
#define ENTITY_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#include "model.h"
#include "shader_manager.h"

struct Orientation
{
    glm::vec3 origin;
    glm::mat4 rotation;
    glm::mat4 scale;
};

// Collection of physical/graphical elements of a world object
class Entity
{
  public:
    // init with null members, methods should null check and fail safe
    Entity();
    // Caller must std::move pointer to entity or use temp instance.
    // Once Entity owns a model caller loses direct access.
    Entity(std::unique_ptr<Model> init_model);

    // Feeds model transforms and material info and then draws model meshes. Camera/Lighting/reflection/scene information should already be set.
    bool invoke_draw(ShaderManager& sm);
    glm::mat4 get_model_transform();

    void set_origin(glm::vec3 coord);
    glm::vec3 get_origin();
    void translate(glm::vec3 displace);
    void rotate(float rads, glm::vec3 axis);    // should euler angles be maintained for some use?
    void set_uniform_scale(float factor);

    // Caller must std::move pointer to entity or use temp instance.
    // Once Entity owns a model caller loses direct access.
    void reset_graphics_model(std::unique_ptr<Model> new_graphics_model);
    bool reset_graphics_model_environment_maps(unsigned int new_environment_map_id = 0);

  protected:
    void init_orientation();

    std::unique_ptr<Model> graphics_model;
    //Collider physics_model;

    // all entities, regardless of physics, graphics, or lack thereof will have basic stats
    // graphics and physics models will have to calibrate to this origin
    glm::vec3 origin;
    glm::mat4 rotation;
    glm::mat4 scale;
};

Entity::Entity()
{
    init_orientation();
}

Entity::Entity(std::unique_ptr<Model> init_model) :
    graphics_model(std::move(init_model))
{
    init_orientation();
}

void Entity::init_orientation()
{
    origin = glm::vec3(0.0f);
    rotation = glm::mat4(1.0f);
    scale = glm::mat4(1.0f);
}

bool Entity::invoke_draw(ShaderManager& sm)
{
    if (!graphics_model)
    {
        std::cerr << "Cannot invoke_draw on Entity with no Model." << std::endl;
        return false;
    }

    sm.activate();
    sm.setMat4("model_to_world", get_model_transform());
    graphics_model->invoke_draw(sm);

    return true;
}

glm::mat4 Entity::get_model_transform()
{
    glm::mat4 model_to_world = glm::mat4(1.0f);
    model_to_world = glm::translate(model_to_world, origin);
    model_to_world = model_to_world * rotation * scale;
    return model_to_world;
}

void Entity::set_origin(glm::vec3 coord)
{
    origin = coord;
}

glm::vec3 Entity::get_origin()
{
    return origin;
}

void Entity::translate(glm::vec3 displace)
{
    origin += displace;
}

void Entity::rotate(float rads, glm::vec3 axis)
{
    rotation = glm::rotate(rotation, rads, axis);
}

void Entity::set_uniform_scale(float factor)
{
    scale = glm::mat4(factor);
    scale[3][3] = 1.0f;
}

void Entity::reset_graphics_model(std::unique_ptr<Model> new_graphics_model)
{
    graphics_model = std::move(new_graphics_model);
}

bool Entity::reset_graphics_model_environment_maps(unsigned int new_environment_map_id)
{
    if (!graphics_model)
    {
        std::cerr << "Cannot modelfy environment maps on null Model." << std::endl;
        return false;
    }

    graphics_model->reset_all_environment_maps(new_environment_map_id);
    return true;
}

#endif // ENTITY_H