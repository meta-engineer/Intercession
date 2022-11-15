#ifndef VERTEX_GROUP_H
#define VERTEX_GROUP_H

//#include "intercession_pch.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "logging/pleep_log.h"

namespace pleep
{
    // basic grouping of Vertex buffer objects, they are immutable once created
    // less sophisticated than a full Mesh
    class VertexGroup
    {
    public:
        // accepts pointers to vertex data, pointers only need to be valid for duration of constructor
        // then they are loaded into VertexBuffers
        VertexGroup(
            const float* vertices, const unsigned long long numVertices,
            const unsigned int* indices, const unsigned long long numIndices, 
            const unsigned int* attribs, const unsigned long long numAttribs)
            : m_numIndices(numIndices)
        {
            glGenVertexArrays(1, &VAO_ID);
            glBindVertexArray(VAO_ID);

            glGenBuffers(1, &VBO_ID);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
            glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(float), vertices, GL_STATIC_DRAW);

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
            for (unsigned int i = 0; i < numAttribs; i++) {
                stride += attribs[i];
            }

            for (unsigned int i = 0; i < numAttribs; i++)
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

        ~VertexGroup()
        {
            glDeleteBuffers(1, &EBO_ID);
            glDeleteBuffers(1, &VBO_ID);
            glDeleteVertexArrays(1, &VAO_ID);
        }

        void invoke_draw()
        {
            glBindVertexArray(VAO_ID);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_numIndices), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

    private:
        // draw call size
        unsigned long long m_numIndices;

        unsigned int VAO_ID;
        unsigned int VBO_ID;
        unsigned int EBO_ID;
        
    };
}

#endif // VERTEX_GROUP_H