#include "model.h"

#define STB_IMAGE_IMPLEMENTATION // must define before include?
#include <stb_image.h>

#include "logging/pleep_log.h"

namespace pleep
{
    Model::Model(std::string path)
    {
        _load_model(path.c_str());
    }

    Model::Model(std::vector<Mesh> meshes)
        : meshes(meshes)
    {}

    Model::Model(Mesh mesh)
    {
        meshes.push_back(mesh);
    }

    Model::~Model()
    {
        // Free my meshes GPU memory before deleting them
        // My meshes should be unique, and their textures should be unique
        //   (between models, not necessarily inside a model)
        // IF textures/meshes become shared across models this must be smart deletion
        for (unsigned int i = 0; i < meshes.size(); i++)
        {
            while (meshes[i].textures.size() > 0)
            {
                glDeleteTextures(1, &(meshes[i].textures.back().id));
                meshes[i].textures.pop_back();
            }
            
            // cube maps are shared between skybox and model reflections so we can't delete it...
            //glDeleteTextures(1, &(meshes[i].environment_map.id));
            //meshes[i].environment_map.id = 0;
        }
    }

    void Model::invoke_draw(ShaderManager& sm)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].invoke_draw(sm);
    }

    void Model::invoke_instanced_draw(ShaderManager& sm, size_t amount)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].invoke_instanced_draw(sm, amount);
    }

    void Model::reset_all_environment_cubemaps(unsigned int newEnvironmentCubemap_id)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].reset_environment_cubemap(newEnvironmentCubemap_id);
    }

    void Model::setup_all_instance_transform_attrib_arrays(unsigned int offset)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].setup_instance_transform_attrib_array(offset);
    }


    void Model::_load_model(std::string path)
    {
        Assimp::Importer importer;
        // aiProcess_GenSmoothNormals ?
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
        {
            PLEEPLOG_ERROR("Assimp failed to load: " + std::string(importer.GetErrorString()));
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));

        _process_node(scene->mRootNode, scene);
    }

    void Model::_process_node(aiNode *node, const aiScene *scene)
    {
        // load all meshes from this level
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(_process_mesh(mesh, scene));
        }
        // recurse to children
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            _process_node(node->mChildren[i], scene);
        }
    }

    Mesh Model::_process_mesh(aiMesh *mesh, const aiScene *scene)
    {
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture>      textures;

        // vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            // we have to manually "cast" the assimp types
            // vertex coordinates
            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;

            // normals
            if (mesh->HasNormals())
            {
                vertex.normal.x = mesh->mNormals[i].x;
                vertex.normal.y = mesh->mNormals[i].y;
                vertex.normal.z = mesh->mNormals[i].z;
            }

            // testure coordinates
            if (mesh->mTextureCoords[0])
            {
                // Assuming we only use 1 set of texture coords (up to 8)
                vertex.texCoord.x = mesh->mTextureCoords[0][i].x;
                vertex.texCoord.y = mesh->mTextureCoords[0][i].y;

                // tangent to face
                vertex.tangent.x = mesh->mTangents[i].x;
                vertex.tangent.y = mesh->mTangents[i].y;
                vertex.tangent.z = mesh->mTangents[i].z;
            }
            else
            {
                vertex.texCoord = glm::vec2(0.0f, 0.0f);
            }

            // setup bone data as default (all disabled)
            set_vertex_bone_data_to_default(vertex);

            vertices.push_back(vertex);
        }

        // setup all bone data (iterating by bones, not by verticies like above)
        _extract_bone_weight_for_vertices(vertices, mesh);

        // indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace tri = mesh->mFaces[i];
            for (unsigned int j = 0; j < tri.mNumIndices; j++)
                indices.push_back(tri.mIndices[j]);
        }

        // material textures
        if (mesh->mMaterialIndex >= 0)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            std::vector<Texture> diffuseMaps = _load_material_textures(material, aiTextureType_DIFFUSE, TextureType::diffuse_map);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            std::vector<Texture> specularMaps = _load_material_textures(material, aiTextureType_SPECULAR, TextureType::specular_map);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            // for .obj normal map is in aiTextureType_HEIGHT. (map_bump attribute)
            // is this the same for all formats? what about when I acually want a height map?
            std::vector<Texture> normalMaps = _load_material_textures(material, aiTextureType_HEIGHT, TextureType::normal_map);
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            // TODO: height maps for models?
            // aiTextureType_AMBIENT
        }
        
        return Mesh(vertices, indices, textures);
    }


    std::vector<Texture> Model::_load_material_textures(aiMaterial *mat, aiTextureType type, TextureType tType)
    {
        std::vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            // check if already loaded
            bool isNewTexture = true;
            for (unsigned int j = 0; j < texturesLoaded.size(); j++)
            {
                if (std::strcmp(texturesLoaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(texturesLoaded[j]);
                    isNewTexture = false;
                    break;
                }
            }
            if (isNewTexture)
            {
                Texture texture;
                texture.id = load_gl_texture(str.C_Str(), directory, DOES_TEXTURE_USE_GAMMA(tType));
                texture.type = tType;
                texture.path = str.C_Str();
                textures.push_back(texture);
                texturesLoaded.push_back(texture);
            }
        }

        return textures;
    }

    unsigned int Model::load_gl_texture(std::string filename, const std::string& path, bool gammaCorrect)
    {
        std::string filepath = filename;
        if (!path.empty())
            filepath = path + '/' + filepath;

        unsigned int texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        // TODO: passing wrapping options?
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
                internalFormat = gammaCorrect ? GL_SRGB : GL_RGB;
                dataFormat = GL_RGB;
            }
            else if (texChannels == 4)
            {
                internalFormat = gammaCorrect ? GL_SRGB_ALPHA : GL_RGBA;
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
            PLEEPLOG_ERROR("MODEL::load_gl_texture Failed to load texture: " + filepath);
        }
        stbi_image_free(texData);

        return texture_id;
    }

    unsigned int Model::load_gl_cubemap_texture(std::vector<std::string> filenames, bool gammaCorrect)
    {
        unsigned int texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

        // options for cube_maps
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // data
        int texWidth, texHeight, texChannels;
        for (unsigned int i = 0; i < std::min(filenames.size(), (size_t)6); i++)
        {
            unsigned char *texData = stbi_load(filenames[i].c_str(), &texWidth, &texHeight, &texChannels, 0);
            if (texData)
            {
                GLenum internalFormat;
                GLenum dataFormat;
                if (texChannels == 3)
                {
                    internalFormat = gammaCorrect ? GL_SRGB : GL_RGB;
                    dataFormat = GL_RGB;
                }
                else if (texChannels == 4)
                {
                    internalFormat = gammaCorrect ? GL_SRGB_ALPHA : GL_RGBA;
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
                PLEEPLOG_ERROR("MODEL::load_gl_texture Failed to load texture: " + filenames[i]);
            }
            stbi_image_free(texData);
        }

        // clear
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        return texture_id;
    }

    std::map<std::string, BoneInfo>& Model::get_bone_info_map()
    {
        return boneInfoMap;
    }

    int& Model::get_num_bones()
    {
        return boneIdCounter;
    }

    void Model::set_vertex_bone_data_to_default(Vertex& vertex)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
            vertex.boneIds[i] = -1;
            vertex.boneWeights[i] = 0.0f;
        }
    }

    void Model::set_vertex_bone_data(Vertex& vertex, int boneId, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            if (vertex.boneIds[i] < 0)
            {
                vertex.boneWeights[i] = weight;
                vertex.boneIds[i] = boneId;
                break;
            }
        }
    }

    void Model::_extract_bone_weight_for_vertices(std::vector<Vertex>& vertices, aiMesh* mesh)
    {
        // parse through assimp bone list
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
        {
            int boneId = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

            // if this bone isn't in our internal list
            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                BoneInfo newBone;
                newBone.id = boneIdCounter;
                // we have to manually cast these math types, as in _process_mesh with vec3's
                // i dont think there is row,col indexing
                newBone.model_to_bone = assimp_converters::convert_matrix(mesh->mBones[boneIndex]->mOffsetMatrix);

                // care for collisions? this depends on external model data
                boneInfoMap[boneName] = newBone;

                boneId = newBone.id;
                boneIdCounter++;
            }
            else
            {
                boneId = boneInfoMap[boneName].id;
            }

            // bone was either created or fetched
            assert(boneId != -1);

            // fetch corresponding weight for "this" bone
            aiVertexWeight* aiWeights = mesh->mBones[boneIndex]->mWeights;
            unsigned int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (unsigned int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                // our vertices array matches indices in weights?!
                unsigned int aiVertexId = aiWeights[weightIndex].mVertexId;
                // this is an aiReal ?
                float boneWeight = aiWeights[weightIndex].mWeight;

                // check for weirdness
                assert(aiVertexId < vertices.size());

                set_vertex_bone_data(vertices[aiVertexId], boneId, boneWeight);
            }
        }
    }
}