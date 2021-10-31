
#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION // must define before include?
#include <stb_image.h>

#include "mesh.h"

class Model
{
  public:
    // use assimp to load from file
    Model(char* path);
    // pass pre-constructed meshes
    Model(std::vector<Mesh> meshes);
    // to use a single pre-constructed mesh
    Model(Mesh mesh);
    // shader must be activated before calling invoke_draw() !!!!
    void invoke_draw(ShaderManager& sm);

    // for use as
    static int loadGLTexture(std::string filename, const std::string& path = "");

  private:
    // TODO: heirarchy of meshes is not preserved
    std::vector<Mesh> meshes;
    // to find associated files
    std::string directory;
    // should this be static so all model instances share it?
    std::vector<Texture> textures_loaded;

    void loadModel(std::string path);
    // recursively process from scene->mRootNode
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, TextureType tType);
};

Model::Model(char* path)
{
    loadModel(path);
}

Model::Model(std::vector<Mesh> meshes)
    : meshes(meshes)
{}

Model::Model(Mesh mesh)
{
    meshes.push_back(mesh);
}

void Model::invoke_draw(ShaderManager& sm)
{
    for(unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].invoke_draw(sm);
}

void Model::loadModel(std::string path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {
        std::cerr << "Assimp failed to load: " << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    // load all meshes from this level
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    // recurse to children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
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
            vertex.tex_coord.x = mesh->mTextureCoords[0][i].x;
            vertex.tex_coord.y = mesh->mTextureCoords[0][i].y;
        }
        else
        {
            vertex.tex_coord = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

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

        std::vector<Texture> diffuse_maps = loadMaterialTextures(material, aiTextureType_DIFFUSE, TextureType::diffuse_map);
        textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());

        std::vector<Texture> specular_maps = loadMaterialTextures(material, aiTextureType_SPECULAR, TextureType::specular_map);
        textures.insert(textures.end(), specular_maps.begin(), specular_maps.end());

        // for later :)
        //std::vector<Texture> normal_maps = loadMaterialTextures(material, aiTextureType_HEIGHT, TextureType::normal_map);
        //textures.insert(textures.end(), height_maps.begin(), height_maps.end());
    }
    
    return Mesh(vertices, indices, textures);
}


std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, TextureType tType)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        // check if already loaded
        bool is_new_texture = true;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
            {
                textures.push_back(textures_loaded[j]);
                is_new_texture = false;
                break;
            }
        }
        if (is_new_texture)
        {
            Texture texture;
            texture.id = loadGLTexture(str.C_Str(), directory);
            texture.type = tType;
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
    }

    return textures;
}

int Model::loadGLTexture(std::string filename, const std::string& path)
{
    std::string filepath = filename;
    if (!path.empty())
        filepath = path + '/' + filepath;

    unsigned int texture_ID;
    glGenTextures(1, &texture_ID);
    glBindTexture(GL_TEXTURE_2D, texture_ID);

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
        GLenum format;
        if (texChannels == 3)
            format = GL_RGB;
        else if (texChannels == 4)
            format = GL_RGBA;
        else // if (texChannels == 1)
            format = GL_RED;

        glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, texData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "MODEL::loadGLTexture Failed to load texture: " << filepath << std::endl;
    }
    stbi_image_free(texData);

    return texture_ID;
}

#endif // MODEL_H