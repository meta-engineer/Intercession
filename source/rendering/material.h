#ifndef MATERIAL_H
#define MATERIAL_H

//#include "intercession_pch.h"
#include <memory>
#include <unordered_map>

#include "rendering/texture.h"

namespace pleep
{
    // Will materials need to be a completely flexable list of textures or have a strict format?
    // STD: diffuse, specular, normal,                displace? emission?
    // PBR: albedo,  metallic, normal, roughness, AO, displace? emission?

    // Material defines a specific collection of textures according to a rendering method
    // Materials are sharable (maintained by ModelLibrary), but we'll assume once a material
    // is created we wont require sharing, mixing, and matching Textures between Materials
    // basic material variation can be handled by options inside 
    // MaterialComponents per entity and/or by shaders.
    // In the rare case of needing a hybridized material we'll have to reload those textures again
    class Material
    {
    public:
        // Must move texture hashmap into Material to avoid auto-destroying the copy
        // movedTextures will be invalid after calling this constructor
        Material(std::unordered_map<TextureType, Texture>&& movedTextures)
            : m_textures(std::move(movedTextures))
        {}
        ~Material() = default;

        // only 1 texture per type? every type represented?
        // map of textureType to Texture:
        // no entry implies null texture (0)
        std::unordered_map<TextureType, Texture> m_textures;

        // Name given for this material
        std::string m_name = "";
        // Filename this material was imported from
        std::string m_sourceFilepath = "";
    };
}

#endif // MATERIAL_H