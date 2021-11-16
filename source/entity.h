
#ifndef ENTITY_H
#define ENTITY_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

#include "model.h"
#include "shader_manager.h"

// Collection of physical/graphical elements of a world object
class Entity
{
  public:
    // init with null members, methods should null check and fail safe
    Entity();
    // Feeds model transforms and material info and then draws model meshes. Camera/Lighting/reflection/scene information should already be set.
    bool invoke_draw(ShaderManager& sm);
    glm::mat4 get_model_transform();

    void set_origin(glm::vec3 coord);
    glm::vec3 get_origin();
    void translate(glm::vec3 displace);
    // should euler angles be maintained for some use?
    void rotate(float rads, glm::vec3 axis);
    void set_uniform_scale(float factor);

    // full access to members?
    std::unique_ptr<Model> graphical_model;

  private:
    //Collider physical_model;

    // all bodies, regardless of physics, graphics, or lack thereof will have basic stats
    // graphics and physics models will have to calibrate to this origin
    glm::vec3 origin;
    glm::mat4 rotation;
    glm::mat4 scale;
};

Entity::Entity()
    : origin(0.0f)
    , rotation(1.0f)
    , scale(1.0f)
{}

bool Entity::invoke_draw(ShaderManager& sm)
{
    if (!graphical_model)
    {
        std::cerr << "Tried to invoke_draw on Entity with no Model." << std::endl;
        return false;
    }

    sm.activate();
    sm.setMat4("model_to_world", get_model_transform());
    graphical_model->invoke_draw(sm);

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


#endif // ENTITY_H