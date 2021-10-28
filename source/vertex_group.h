
#ifndef VERTEX_GROUP_H
#define VERTEX_GROUP_H

#include <iostream>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class VertexGroup
{
  public:
    VertexGroup(float* vertices, unsigned int size_vertices, unsigned int* indices, unsigned int size_indices, unsigned int* attribs, unsigned int size_attribs);
    ~VertexGroup();

    glm::mat4 get_model_transform();
    void set_origin(glm::vec3 coord);
    glm::vec3 get_origin();
    void translate(glm::vec3 displace);
    void rotate(float rads, glm::vec3 axis);
    void set_uniform_scale(float factor);
    void invoke_draw();

  private:
    glm::vec3 origin;
    glm::mat4 rotation; // share some common rotation manipulation from camera?
    glm::mat4 scale;

    // TODO: copy in mesh properties. could be useful to know later? And delete in destructor
    // could be stored in smart containers
    float* vertices;
    unsigned int num_vertices;
    unsigned int* indices;
    unsigned int num_indices;
    unsigned int* attribs;
    unsigned int num_attribs;

    unsigned int VAO_ID;
    unsigned int VBO_ID;
    unsigned int EBO_ID;
};

VertexGroup::VertexGroup(float* vertices, unsigned int size_vertices, unsigned int* indices, unsigned int size_indices, unsigned int* attribs, unsigned int size_attribs)
    : origin(0.0f)
    , rotation(1.0f)
    , scale(1.0f)
    , num_vertices(size_vertices/sizeof(unsigned int))
    , num_indices(size_indices/sizeof(unsigned int))
    , num_attribs(size_attribs/sizeof(unsigned int))
{
    glGenVertexArrays(1, &VAO_ID);
    glBindVertexArray(VAO_ID);

    glGenBuffers(1, &VBO_ID);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
    glBufferData(GL_ARRAY_BUFFER, size_vertices, vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO_ID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_indices, indices, GL_STATIC_DRAW);

    // Set the interpretation of our VBO structure for the vertex shader input
    //  (glVertexAttribPointer takes data from the currently bound vbo)
    //  Arguments:
    //  Attrbute to configure (referenced in shader)
    //  size of attribute
    //  type of contained data
    //  normalize data?
    //  stride (space between consecutive attributes)
    //  offset of data in the buffer
    unsigned int offset = 0;
    unsigned int stride = 0;
    for (int i = 0; i < num_attribs; i++) {
        stride += attribs[i];
    }

    for (int i = 0; i < num_attribs; i++)
    {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, attribs[i], GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset * sizeof(float)));
        offset += attribs[i];
    }

    // clear binds
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

VertexGroup::~VertexGroup()
{
    glDeleteBuffers(1, &EBO_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteVertexArrays(1, &VAO_ID);
}

glm::mat4 VertexGroup::get_model_transform()
{
    glm::mat4 model_to_world = glm::mat4(1.0f);
    model_to_world = glm::translate(model_to_world, origin);
    model_to_world = model_to_world * rotation * scale;
    return model_to_world;
}

void VertexGroup::set_origin(glm::vec3 coord)
{
    origin = coord;
}
glm::vec3 VertexGroup::get_origin()
{
    return origin;
}

void VertexGroup::translate(glm::vec3 displace)
{
    origin += displace;
}

void VertexGroup::rotate(float rads, glm::vec3 axis)
{
    rotation = glm::rotate(rotation, rads, axis);
}

void VertexGroup::set_uniform_scale(float factor)
{
    scale = glm::mat4(factor);
    scale[3][3] = 1.0f;
}

void VertexGroup::invoke_draw()
{
    glBindVertexArray(VAO_ID);
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

#endif // VERTEX_GROUP_H