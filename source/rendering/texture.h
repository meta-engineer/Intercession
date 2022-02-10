#ifndef TEXTURE_H
#define TEXTURE_H

//#include "intercession_pch.h"
#include <string>

namespace pleep
{
    // these will be used as the prefix for shader Material struct members
    enum TextureType
    {
        diffuse_map,
        specular_map,
        cube_map,
        normal_map,
        displace_map
    };
    // TODO: find an elegant was to convert enum to string
    inline std::string TEXTURETYPE_TO_STR(TextureType t)
    {
        switch(t)
        {
            case (TextureType::diffuse_map):
                return "diffuse_map";
            case (TextureType::specular_map):
                return "specular_map";
            case (TextureType::cube_map):
                return "cube_map";
            case (TextureType::normal_map):
                return "normal_map";
            case (TextureType::displace_map):
                return "displace_map";
            default:
                return "error_unmapped_texture_type";
        }
    }

    // what texture types will get gamma correction on model load
    inline bool DOES_TEXTURE_USE_GAMMA(TextureType t)
    {
        switch (t)
        {
            case(TextureType::diffuse_map):
                return true;
            case(TextureType::specular_map):
                return true;
            default:
                return false;
        }
    }

    struct Texture
    {
        unsigned int id;
        TextureType type;
        std::string path; // location where file was loaded from
    };
}

#endif // TEXTURE_H