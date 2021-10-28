
#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"

class Model
{
  public:
    Model(char* path);
    void invokeDraw(ShaderManager& sm);

  private:
    // TODO: heirarchy of meshes is not preserved
    std::vector<Mesh> meshes;
    std::string directory;

    void loadModel(std::string path);
    // recursively process from scene->mRootNode
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};

Model::Model(char* path)
{
    loadModel(path);
}

void Model::invokeDraw(ShaderManager& sm)
{
    for(unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].invokeDraw(sm);
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
        // TODO: heirarchy of meshes is not preserved
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
        ...
        vertices.push_back(vertex);
    }

    // indices
    ...

    // material textures
    if (mesh->mMaterialIndex >= 0)
    {
        ...
    }

    return Mesh(vertices, indices, textures);
}


std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;

    return textures;
}

#endif // MODEL_H