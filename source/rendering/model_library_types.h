#ifndef MODEL_LIBRARY_TYPES_H
#define MODEL_LIBRARY_TYPES_H

//#include "intercession_pch.h"
#include <string>
#include <vector>

#include "rendering/mesh.h"

namespace pleep
{
    namespace ModelLibrary
    {
        // hardcoded supermeshes
        enum class BasicSupermesh
        {
            // cube with 1m side lengths, normals are not interpolated on corners
            cube,
            // quad in x-y plane with 1m side lengths
            quad,
            // quad in x-y plane with vertices at +/- 1
            screen,
            // regular icosahedron with 1m circumdiameter
            icosahedron,
            // geodesic sphere with 1m diameter
            //sphere,
        };
        // Need a matching string value to use m_supermeshMap
        // use <> characters because they are illegal for filesnames and wont cause collisions
        // TODO: find an elegant was to convert enum to string
        inline std::string ENUM_TO_STR(BasicSupermesh b)
        {
            switch(b)
            {
                case (BasicSupermesh::cube):
                    return "<pleep_cube>";
                case (BasicSupermesh::quad):
                    return "<pleep_quad>";
                case (BasicSupermesh::screen):
                    return "<pleep_screen>";
                case (BasicSupermesh::icosahedron):
                    return "<pleep_icosahedron>";
                default:
                    return "";
            }
        }

        // Names of assets guaranteed to be cached after a model import.
        // Also used during import communicate down the pipeline which assets were loaded in current import 
        struct ImportReceipt
        {
            // Name of overall import (also used as deafult base name for unnamed assets)
            std::string importName;
            // location import was read from
            std::string importSourceFilepath;

            // list of all unique Supermesh names from this import
            std::vector<std::string> supermeshNames;
            // each supermeshMaterialsNames entry corresponds with the supermeshName at the same index
            // each supermeshMaterialsNames ENTRY'S entry corresponds with the submesh at the same index
            std::vector<std::vector<std::string>> supermeshMaterialsNames;

            // list of all unique material names from this import
            std::vector<std::string> materialNames;
            // list of all unique armature names from this import
            std::vector<std::string> armatureNames;
            // list of all unique animation names from this import
            std::vector<std::string> animationNames;
        };
    }
}

#endif // MODEL_LIBRARY_TYPES_H