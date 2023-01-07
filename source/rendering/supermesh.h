#ifndef SUPERMESH_H
#define SUPERMESH_H

//#include "intercession_pch.h"
#include <vector>
#include <memory>

#include "rendering/mesh.h"

namespace pleep
{
    // Imported meshes will be divided into seperate submeshes according to materials
    // (so a mesh only ever uses 1 material)
    // A Supermesh is a collection of meshes which belong to 1 3d model
    class Supermesh
    {
    public:
        // init Supermesh with 0 submeshes
        Supermesh() = default;
        // init Supermesh with 1 submesh
        Supermesh(const std::string& name, const std::string& sourceFilepath, std::shared_ptr<Mesh> initSubmesh)
            : m_name(name)
            , m_sourceFilepath(sourceFilepath)
        {
            m_submeshes.push_back(initSubmesh);
        }
        ~Supermesh() = default;

        
        // each mesh manages its own GPU memory
        std::vector<std::shared_ptr<Mesh>> m_submeshes;

        std::string m_name;
        std::string m_sourceFilepath;
    };
}

#endif // SUPERMESH_H