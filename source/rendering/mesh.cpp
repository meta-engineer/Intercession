#include "mesh.h"

#include "logging/pleep_log.h"

namespace pleep
{
    Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
        : m_vertices(vertices)
        , m_indices(indices)
    {
        _setup();
    }

    Mesh::~Mesh()
    {
        // can only call GL functions if GL has been setup
        if (!m_isGlSetup) return;

        glDeleteBuffers(1, &VAO_ID);
        glDeleteBuffers(1, &VBO_ID);
        glDeleteBuffers(1, &EBO_ID);
    }

    void Mesh::_setup()
    {
        glGenVertexArrays(1, &VAO_ID);
        glBindVertexArray(VAO_ID);

        glGenBuffers(1, &VBO_ID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &EBO_ID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_ID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

        // vertex attribs will always be position, normal, texture from now on so this can be static 3,3,2
        // vertex positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
        // vertex tangents
        glEnableVertexAttribArray(3);	
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

        // bone ids
        glEnableVertexAttribArray(4);
        glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIds));
        // bone weights
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));

        // clear binds
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        m_isGlSetup = true;
    }

    void Mesh::invoke_draw(ShaderManager& sm) const
    {
        if (!m_isGlSetup) return;
        // textures should already be set by relay

        // Save some cycles by assuming sm will be activated?
        UNREFERENCED_PARAMETER(sm);
        //sm.activate();

        glBindVertexArray(VAO_ID);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        //sm.deactivate();
    }

    void Mesh::invoke_instanced_draw(ShaderManager& sm, size_t amount) const
    {
        if (!m_isGlSetup) return;
        if (amount == 0) return;
        // textures should already be set by relay

        // TODO: Save some cycles by assuming sm will be activated?
        UNREFERENCED_PARAMETER(sm);
        //sm.activate();

        glBindVertexArray(VAO_ID);
        glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(amount));
        glBindVertexArray(0);

        //sm.deactivate();
    }

    void Mesh::setup_instance_transform_attrib_array(unsigned int offset)
    {
        if (!m_isGlSetup) return;
        glBindVertexArray(VAO_ID);

        // Meshes already use 0(positions), 1(normals), 2(texture coords), 3(tangents)
        //      optionally 4(boneids), 5(boneweights)
        // so offset = 6 (default) should be used
        // set attrib location [6->9] as transform matrix rows
        glEnableVertexAttribArray(offset + 0);
        glVertexAttribPointer(offset + 0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glVertexAttribDivisor(offset + 0, 1);

        glEnableVertexAttribArray(offset + 1);
        glVertexAttribPointer(offset + 1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(1 * sizeof(glm::vec4)));
        glVertexAttribDivisor(offset + 1, 1);

        glEnableVertexAttribArray(offset + 2);
        glVertexAttribPointer(offset + 2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glVertexAttribDivisor(offset + 2, 1);

        glEnableVertexAttribArray(offset + 3);
        glVertexAttribPointer(offset + 3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));
        glVertexAttribDivisor(offset + 3, 1);

        glBindVertexArray(0);
    }
}