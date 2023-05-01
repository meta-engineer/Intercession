#ifndef TEXTURE_H
#define TEXTURE_H

//#include "intercession_pch.h"
#include <string>
#include <vector>

namespace pleep
{
   // Castable superset of Assimp texture types
   enum TextureType
   {
        // Legacy
        none = 0,
        diffuse,
        specular,
        ambient,
        emissive,
        height,
        normal,
        shininess,
        opacity,
        displacement,
        light_map,
        reflection,

        // PBR
        albedo,
        normal_camera,
        emission,
        metalness,
        roughness,
        ao, // Ambient Occlusion

        unknown,

        // PBR Modifiers
        sheen,
        clearcoat,
        transmission,

        // PLEEP Specific
        cube,   // does this need to be specified, or is a cubemap a type of diffuse map?
        texture_type_count
   };
    // TODO: find a more elegant way to convert enum to string
    // these will be used as the prefix for shader Material struct members
    inline const char* TEXTURETYPE_TO_STR(TextureType t)
    {
        switch(t)
        {
            case (TextureType::diffuse):
                return "diffuse_map";
            case (TextureType::specular):
                return "specular_map";
            case (TextureType::cube):
                return "cube_map";
            case (TextureType::normal):
                return "normal_map";
            case (TextureType::height):
                return "height_map";
            case (TextureType::emissive):
                return "emissive_map";
            default:
                return "error_unmapped_texture_type";
        }
    }

    // what texture types will get gamma correction on model load
    inline bool TEXTURETYPE_USE_GAMMA(TextureType t)
    {
        switch (t)
        {
            case(TextureType::diffuse):
                return true;
            case(TextureType::specular):
                return true;
            case(TextureType::emissive):
                return true;
            default:
                return false;
        }
    }

    // Wrap/manage GL texture handle
    struct Texture
    {
    public:
        // load texture from file into GPU memory using stbi
        Texture(TextureType type, const std::string& filepath);
        // load cubemap texture from multiple files (ordered by GL_TEXTURE_CUBE_MAP_...)
        Texture(const std::vector<std::string> filepaths);
        // copying a Texture would mean GPU memory could be freed by the copy
        Texture(const Texture&) = delete;
        // Texture must free its owned GPU memory
        ~Texture();

        unsigned int get_id() const { return m_id; }
        TextureType get_type() const { return m_type; }
        std::string get_source_filepath() const { return m_sourceFilepath; }

    private:
        unsigned int m_id = 0;                  // gl handle
        TextureType m_type = TextureType::none;
        std::string m_sourceFilepath = "";      // location where file was loaded from
    };
}

#endif // TEXTURE_H