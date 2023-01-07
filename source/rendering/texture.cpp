#include "rendering/texture.h"

#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION // must define before include?
#include <stb_image.h>

#include "logging/pleep_log.h"

namespace pleep
{
    Texture::~Texture()
    {
        glDeleteTextures(1, &m_id);
    }

    Texture::Texture(TextureType type, const std::string& filepath)
    {
        // Auxilary members
        m_type = type;
        m_sourceFilename = filepath;

        // Load GL texture handle
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_2D, m_id);

        // TODO: pass in wrapping options?
        // GL_REPEAT tiles the texture
        // GL_MIRRORED_REPEAT tiles the texture 
        // GL_CLAMP_TO_BORDER uses border parameter (or transparency) outside texture bounds
        // GL_CLAMP_TO_EDGE uses last tex value outside texture bounds
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        // color for non-mapped surface (using GL_CLAMP_TO_BORDER)
        //float borderColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
        //glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // texture mapping options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // texturing data
        int texWidth, texHeight, texChannels;
        // the aiProcess_FlipUVs option to aiImporter.ReadFile() already does this?
        //stbi_set_flip_vertically_on_load(true);
        unsigned char *texData = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, 0);
        if (texData)
        {
            GLenum internalFormat;
            GLenum dataFormat;
            if (texChannels == 3)
            {
                internalFormat = TEXTURETYPE_USE_GAMMA(m_type) ? GL_SRGB : GL_RGB;
                dataFormat = GL_RGB;
            }
            else if (texChannels == 4)
            {
                internalFormat = TEXTURETYPE_USE_GAMMA(m_type) ? GL_SRGB_ALPHA : GL_RGBA;
                dataFormat = GL_RGBA;
            }
            else // if (texChannels == 1)
            {
                internalFormat = dataFormat = GL_RED;
            }

            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, texWidth, texHeight, 0, dataFormat, GL_UNSIGNED_BYTE, texData);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            PLEEPLOG_ERROR("Failed to load texture: " + filepath);
            // delete generated texture and set null values
            glDeleteTextures(1, &m_id);
            m_id = 0;
            m_type = TextureType::none;
            m_sourceFilename = "";
        }
        stbi_image_free(texData);
        
        // clear binds
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    Texture::Texture(const std::vector<std::string> filepaths) 
    {
        if (filepaths.empty())
        {
            // use header defaults
            return;
        }

        // Auxilary members
        m_type = TextureType::cube;
        // use first file as source?
        m_sourceFilename = filepaths.at(0);

        // Load GL texture handle
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_id);

        // options for cube_maps
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // data
        int texWidth, texHeight, texChannels;
        for (unsigned int i = 0; i < std::min(filepaths.size(), (size_t)6); i++)
        {
            unsigned char *texData = stbi_load(filepaths[i].c_str(), &texWidth, &texHeight, &texChannels, 0);
            if (texData)
            {
                GLenum internalFormat;
                GLenum dataFormat;
                if (texChannels == 3)
                {
                    internalFormat = TEXTURETYPE_USE_GAMMA(m_type) ? GL_SRGB : GL_RGB;
                    dataFormat = GL_RGB;
                }
                else if (texChannels == 4)
                {
                    internalFormat = TEXTURETYPE_USE_GAMMA(m_type) ? GL_SRGB_ALPHA : GL_RGBA;
                    dataFormat = GL_RGBA;
                }
                else // if (texChannels == 1)
                {
                    internalFormat = dataFormat = GL_RED;
                }

                glTexImage2D(
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                    0, internalFormat, texWidth, texHeight, 0, dataFormat, GL_UNSIGNED_BYTE, texData
                );
            }
            else
            {
                PLEEPLOG_ERROR("Failed to load texture for cubemap: " + filepaths[i]);
                // unsure of how to handle this, for now just let that cube face be blank?
            }
            stbi_image_free(texData);
        }

        // clear binds
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }


}