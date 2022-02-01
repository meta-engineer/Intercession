#ifndef VERTEX_GROUP_H
#define VERTEX_GROUP_H

#include <iostream>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class VertexGroup
{
  public:
    VertexGroup(
        const float* vertices, const unsigned int numVertices,
        const unsigned int* indices, const unsigned int numIndices, 
        const unsigned int* attribs, const unsigned int numAttribs
    );
    ~VertexGroup();

    glm::mat4 get_model_transform();
    void set_origin(glm::vec3 coord);
    glm::vec3 get_origin();
    void translate(glm::vec3 displace);
    void rotate(float rads, glm::vec3 axis);
    void set_uniform_scale(float factor);
    void invoke_draw();

  private:
    glm::vec3 m_origin;
    glm::mat4 m_rotation; // share some common m_rotation manipulation from camera?
    glm::mat4 m_scale;

    // TODO: copy in mesh properties. could be useful to know later? And delete in destructor
    // could be stored in smart containers
    float* m_vertices;
    unsigned int m_numVertices;
    unsigned int* m_indices;
    unsigned int m_numIndices;
    unsigned int* m_attribs;
    unsigned int m_numAttribs;

    unsigned int VAO_ID;
    unsigned int VBO_ID;
    unsigned int EBO_ID;
};

VertexGroup::VertexGroup(
        const float* vertices, const unsigned int numVertices,
        const unsigned int* indices, const unsigned int numIndices, 
        const unsigned int* attribs, const unsigned int numAttribs)
    : m_origin(0.0f)
    , m_rotation(1.0f)
    , m_scale(1.0f)
    , m_numVertices(numVertices)
    , m_numIndices(numIndices)
    , m_numAttribs(numAttribs)
{
    glGenVertexArrays(1, &VAO_ID);
    glBindVertexArray(VAO_ID);

    glGenBuffers(1, &VBO_ID);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
    glBufferData(GL_ARRAY_BUFFER, m_numVertices * sizeof(float), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EBO_ID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_numIndices * sizeof(unsigned int), indices, GL_STATIC_DRAW);

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
    for (unsigned int i = 0; i < m_numAttribs; i++) {
        stride += attribs[i];
    }

    for (unsigned int i = 0; i < m_numAttribs; i++)
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
    model_to_world = glm::translate(model_to_world, m_origin);
    model_to_world = model_to_world * m_rotation * m_scale;
    return model_to_world;
}

void VertexGroup::set_origin(glm::vec3 coord)
{
    m_origin = coord;
}
glm::vec3 VertexGroup::get_origin()
{
    return m_origin;
}

void VertexGroup::translate(glm::vec3 displace)
{
    m_origin += displace;
}

void VertexGroup::rotate(float rads, glm::vec3 axis)
{
    m_rotation = glm::rotate(m_rotation, rads, axis);
}

void VertexGroup::set_uniform_scale(float factor)
{
    m_scale = glm::mat4(factor);
    m_scale[3][3] = 1.0f;
}

void VertexGroup::invoke_draw()
{
    glBindVertexArray(VAO_ID);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

#endif // VERTEX_GROUP_H