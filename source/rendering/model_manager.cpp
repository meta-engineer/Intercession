#include "rendering/model_manager.h"

#include <cassert>

#include "logging/pleep_log.h"
#include "rendering/assimp_converters.h"

namespace pleep
{
    ModelManager::ImportReceipt ModelManager::import(const std::string filepath)
    {
        // hardcoded assets need to be able to use the same import pathway as 
        // other supermeshes for ambiguous deserialization
        if (filepath == ModelManager::ENUM_TO_STR(BasicSupermeshType::cube))
        {
            this->m_supermeshMap[filepath] = this->_build_cube_supermesh();
            return ImportReceipt{filepath, filepath, {filepath}};
        }
        else if (filepath == ModelManager::ENUM_TO_STR(BasicSupermeshType::quad))
        {
            this->m_supermeshMap[filepath] = this->_build_quad_supermesh();
            return ImportReceipt{filepath, filepath, {filepath}};
        }
        else if (filepath == ModelManager::ENUM_TO_STR(BasicSupermeshType::screen))
        {
            this->m_supermeshMap[filepath] = this->_build_screen_supermesh();
            return ImportReceipt{filepath, filepath, {filepath}};
        }
        else if (filepath == ModelManager::ENUM_TO_STR(BasicSupermeshType::icosahedron))
        {
            this->m_supermeshMap[filepath] = this->_build_icosahedron_supermesh();
            return ImportReceipt{filepath, filepath, {filepath}};
        }
        else if (filepath == ModelManager::ENUM_TO_STR(BasicSupermeshType::vector))
        {
            this->m_supermeshMap[filepath] = this->_build_vector_supermesh();
            return ImportReceipt{filepath, filepath, {filepath}};
        }

        // TODO: Unit testing lmao
        const size_t delimiterIndex = filepath.find_last_of("/\\");
        std::string directory = ".";
        std::string filename = filepath;
        if (delimiterIndex != std::string::npos)
        {
            directory = filepath.substr(0, delimiterIndex);
            filename = filepath.substr(delimiterIndex + 1);
        }
        std::string filestem = filename.substr(0, filename.find_last_of("."));
        //PLEEPLOG_DEBUG("Loading model " + filename + " (" + filestem + ") from " + directory);

        // load in model file (it is up to user to prevent redundant import calls)
        Assimp::Importer importer;
        // aiProcess_GenSmoothNormals ?
        const aiScene *scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PopulateArmatureData);
        if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
        {
            PLEEPLOG_ERROR("Assimp failed to load '" + filepath + "': " + std::string(importer.GetErrorString()));
            return ImportReceipt{};
        }

        // ***********************************
        //debug_scene(scene);
        //return ImportReceipt{};
        // ***********************************

        // check for possible assets and load into receipt
        ImportReceipt scan = this->_scan_scene(scene, filestem);
        scan.importSourceFilepath = filepath;
        //debug_receipt(scan);
        
        // process materials and armatures first for meshes to reference
        this->_process_materials(scene, scan, directory);
        this->_process_armatures(scene, scan);
        this->_process_supermesh(scene, scan);
        this->_process_animations(scene, scan);

        // return only unique assets
        ImportReceipt receipt = this->_scan_scene(scene, filestem, false);
        receipt.importSourceFilepath = filepath;
        //debug_receipt(receipt);
        // now all assets are cached, user can call fetch methods using receipt names
        return receipt;
    }

    bool ModelManager::create_material(const std::string& name, const std::unordered_map<TextureType, std::string>& textureDict) 
    {
        auto materialIt = this->m_materialMap.find(name);
        if (materialIt != this->m_materialMap.end())
        {
            PLEEPLOG_WARN("Could not create material " + name + " because that name is already taken");
            return false;
        }

        std::unordered_map<TextureType, Texture> newTextures;
        for (auto texEntry : textureDict)
        {
            // weirdness to pass Texture constructor parameters
            newTextures.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(static_cast<TextureType>(texEntry.first)),
                std::forward_as_tuple(static_cast<TextureType>(texEntry.first), texEntry.second)
            );
        }

        std::shared_ptr<Material> newMat = std::make_shared<Material>(std::move(newTextures));
        newMat->m_name = name;
        this->m_materialMap[name] = newMat;
        return true;
    }

    std::shared_ptr<const Supermesh> ModelManager::fetch_supermesh(const std::string& name)
    {
        auto meshIt = this->m_supermeshMap.find(name);
        if (meshIt == this->m_supermeshMap.end())
        {
            PLEEPLOG_WARN("Supermesh " + name + " is not cached.");
            return nullptr;
        }
        else
        {
            return meshIt->second;
        }
    }

    std::shared_ptr<const Material> ModelManager::fetch_material(const std::string& name)
    {
        auto materialIt = this->m_materialMap.find(name);
        if (materialIt == this->m_materialMap.end())
        {
            PLEEPLOG_WARN("Material " + name + " is not cached.");
            return nullptr;
        }
        else
        {
            return materialIt->second;
        }
    }

    std::shared_ptr<Armature> ModelManager::fetch_armature(const std::string& name)
    {
        auto armatureIt = this->m_armatureMap.find(name);
        if (armatureIt == this->m_armatureMap.end())
        {
            PLEEPLOG_WARN("Armature " + name + " is not cached.");
            return nullptr;
        }
        else
        {
            // return copy (copy constructor must be a deep copy)
            return std::make_shared<Armature>(*(armatureIt->second));
        }
    }

    std::shared_ptr<const AnimationSkeletal> ModelManager::fetch_animation(const std::string& name)
    {
        auto animationIt = this->m_animationMap.find(name);
        if (animationIt == this->m_animationMap.end())
        {
            PLEEPLOG_WARN("Material " + name + " is not cached.");
            return nullptr;
        }
        else
        {
            return animationIt->second;
        }
    }
    
    std::shared_ptr<const Supermesh> ModelManager::fetch_supermesh(const ModelManager::BasicSupermeshType id)
    {
        std::string polyName = ModelManager::ENUM_TO_STR(id);
        if (polyName == "") return nullptr;
        
        // check if basic supermesh is in cache from previous call
        auto meshIt = this->m_supermeshMap.find(polyName);
        if (meshIt != this->m_supermeshMap.end())
        {
            return meshIt->second;
        }

        // otherwise, since "filepath" and name are identical we can import it
        this->import(polyName);
        // and then return it in this single call
        return this->m_supermeshMap[polyName];
    }

    void ModelManager::clear_unused()
    {
        PLEEPLOG_WARN("NO IMPLEMENTATION!");
        // Classic removing from dynamic container problem
        /*
                this->m_meshMap.erase(
                    std::remove_if(
                        this->m_meshMap.begin(),
                        this->m_meshMap.end(),
                        [](std::pair<std::string, std::shared_ptr<const Mesh>> meshIt) {
                            if (meshIt.second.use_count() > 1) return false;
                            return true;
                        }
                    ),
                    this->m_meshMap.end()
                );

                this->m_materialMap.erase(
                    std::remove_if(
                        this->m_materialMap.begin(),
                        this->m_materialMap.end(),
                        [](std::pair<std::string, std::shared_ptr<const Material>> materialIt) {
                            if (materialIt.second.use_count() > 1) return false;
                            return true;
                        }
                    ),
                    this->m_materialMap.end()
                );

                this->m_armatureMap.erase(
                    std::remove_if(
                        this->m_armatureMap.begin(),
                        this->m_armatureMap.end(),
                        [](std::pair<std::string, std::shared_ptr<Armature>> armatureIt) {
                            if (armatureIt.second.use_count() > 1) return false;
                            return true;
                        }
                    ),
                    this->m_armatureMap.end()
                );

                this->m_animationMap.erase(
                    std::remove_if(
                        this->m_animationMap.begin(),
                        this->m_animationMap.end(),
                        [](std::pair<std::string, std::shared_ptr<const AnimationSkeletal>> animationIt) {
                            if (animationIt.second.use_count() > 1) return false;
                            return true;
                        }
                    ),
                    this->m_animationMap.end()
                );
         */
    }

    void ModelManager::clear_all()
    {
        this->m_supermeshMap.clear();
        this->m_materialMap.clear();
        this->m_armatureMap.clear();
        this->m_animationMap.clear();
    }
    
    ModelManager::ImportReceipt ModelManager::_scan_scene(const aiScene* scene, const std::string& nameDefault, const bool omitDuplicates) 
    {
        ImportReceipt receipt;
        receipt.importName = nameDefault;

        // all materials should be directly in scene, and have unique named (within this import)
        for (unsigned int i = 0; i < scene->mNumMaterials; i++)
        {
            std::string matName = scene->mMaterials[i]->GetName().C_Str();
            if (matName.empty())
            {
                matName = receipt.importName + "_material_" + std::to_string(i);
            }
            
            // check uniqueness
            if (omitDuplicates && m_materialMap.find(matName) != m_materialMap.end())
            {
                PLEEPLOG_WARN("Could not import " + matName + " it already exists");
                // we could try to generate a default name here?
                continue;
            }

            receipt.materialNames.push_back(matName);
        }

        // all animations should be directly in scene
        for (unsigned int i = 0; i < scene->mNumAnimations; i++)
        {
            std::string animName = scene->mAnimations[i]->mName.C_Str();
            if (animName.empty())
            {
                animName = receipt.importName + "_animation_" + std::to_string(i);
            }

            // check uniqueness
            if (omitDuplicates && m_animationMap.find(animName) != m_animationMap.end())
            {
                PLEEPLOG_WARN("Could not import " + animName + " it already exists");
                // we could try to generate a default name here? unless this already is the deafult name
                continue;
            }

            receipt.animationNames.push_back(animName);
        }

        // can scene not have root node?
        // scene.h::255 > "There will always be at least the root node if the import was successful"
        assert(scene->mRootNode);
        
        // Assume only 1 supermesh, the root node
        std::string supermeshName = scene->mRootNode->mName.C_Str();
        if (supermeshName.empty())
        {
            supermeshName = receipt.importName + "_supermesh";
        }
        // check uniqueness
        if (omitDuplicates && m_supermeshMap.find(supermeshName) != m_supermeshMap.end())
        {
            PLEEPLOG_WARN("Could not import " + supermeshName + " it already exists");
            // we could try to generate a default name here? unless this already is the default name
        }
        else // omitBuplicates == false
        {
            receipt.supermeshName = supermeshName;
        }

        // with no supermesh, no need to get supermeshSubmeshes, supermeshMaterials, or armatures
        if (receipt.supermeshName.empty())
        {
            return receipt;
        } 

        // Assume submeshes are children of root node who have >0 meshes
        // Assume armatures are children of root node who have =0 meshes
        for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++)
        {
            if (scene->mRootNode->mChildren[i]->mNumMeshes > 0)
            {
                for (unsigned int m = 0; m < scene->mRootNode->mChildren[i]->mNumMeshes; m++)
                {
                    aiMesh* submesh = scene->mMeshes[scene->mRootNode->mChildren[i]->mMeshes[m]];
                    receipt.supermeshSubmeshNames.push_back(submesh->mName.C_Str());

                    aiMaterial* material = scene->mMaterials[submesh->mMaterialIndex];
                    receipt.supermeshMaterialNames.push_back(material->GetName().C_Str());
                }
            }
            else
            {
                std::string armatureName = scene->mRootNode->mChildren[i]->mName.C_Str();
                if (armatureName.empty())
                {
                    armatureName = receipt.importName + "_armature_" + std::to_string(i);
                }
                // check uniqueness
                if (omitDuplicates && m_armatureMap.find(armatureName) != m_armatureMap.end())
                {
                    PLEEPLOG_WARN("Could not import " + armatureName + " it already exists");
                    // we could try to generate a default name here? unless this already is the deafult name
                    continue;
                }
                receipt.armatureNames.push_back(armatureName);
            }
        }

        return receipt;
    }

    
    void ModelManager::_process_materials(const aiScene *scene, const ImportReceipt& receipt, const std::string directory)
    {
        // load all materials from scene
        for (unsigned int i = 0; i < scene->mNumMaterials; i++)
        {
            aiMaterial *material = scene->mMaterials[i];
            std::string materialName = std::string(material->GetName().C_Str());
            if (materialName.empty())
            {
                materialName = receipt.importName + "_material_" + std::to_string(i);
            }
            // check if key was validated by scan
            if (std::find(receipt.materialNames.begin(), receipt.materialNames.end(), materialName) == receipt.materialNames.end()) continue;

            m_materialMap[materialName] = _build_material(material, materialName, directory);
            m_materialMap[materialName]->m_name = materialName;
            m_materialMap[materialName]->m_sourceFilepath = receipt.importSourceFilepath;
        }
    }

    std::shared_ptr<Material> ModelManager::_build_material(const aiMaterial *material, const std::string& materialName, const std::string& directory)
    {
        //PLEEPLOG_DEBUG("Loading material: " + materialName);
        UNREFERENCED_PARAMETER(materialName);

        std::unordered_map<TextureType, Texture> loadedTextures;
        // for each aiTextureType in material get (only first?) texture filename

        // aiTextureType should cast directly to pleep::TextureType
        for (unsigned int type = aiTextureType::aiTextureType_NONE; type <= aiTextureType::aiTextureType_TRANSMISSION; type++)
        {
            for (unsigned int i = 0; i < material->GetTextureCount(static_cast<aiTextureType>(type)); i++)
            {
                aiString str;
                material->GetTexture(static_cast<aiTextureType>(type), i, &str);
                // should check for texture already loaded earlier in this model?
                // if it is a duplicate, do we want to load twice anyway to not overlap gpu memory owners?

                if (i > 0)
                {
                    PLEEPLOG_WARN("Found more than one texture of type " + std::to_string(type) + " in material " + std::string(materialName) + " with name " + std::string(str.C_Str()) + " Ignoring...");
                    continue;
                }

                std::string filepath = str.C_Str();
                if (!directory.empty())
                    filepath = directory + '/' + filepath;
                //PLEEPLOG_DEBUG("Loading texture: " + filepath);

                // weirdness to pass Texture constructor parameters
                loadedTextures.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(static_cast<TextureType>(type)),
                    std::forward_as_tuple(static_cast<TextureType>(type), filepath)
                );
            }
        }

        return std::make_shared<Material>(std::move(loadedTextures));
    }

    void ModelManager::_process_armatures(const aiScene *scene, const ImportReceipt& receipt)
    {
        for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++)
        {
            if (scene->mRootNode->mChildren[i]->mNumMeshes > 0) continue;

            std::string armatureName = scene->mRootNode->mChildren[i]->mName.C_Str();
            if (armatureName.empty())
            {
                armatureName = receipt.importName + "_armature_" + std::to_string(i);
            }
            // check if key was validated by scan
            if (std::find(receipt.armatureNames.begin(), receipt.armatureNames.end(), armatureName) == receipt.armatureNames.end()) continue;

            m_armatureMap[armatureName] = _build_armature(scene->mRootNode->mChildren[i], armatureName);
            m_armatureMap[armatureName]->m_sourceFilepath = armatureName;
            m_armatureMap[armatureName]->m_sourceFilepath = receipt.importSourceFilepath;
        }
    }

    std::shared_ptr<Armature> ModelManager::_build_armature(const aiNode* node, const std::string& armatureName)
    {
        //PLEEPLOG_DEBUG("Loading armature: " + armatureName);
        UNREFERENCED_PARAMETER(armatureName);
        std::vector<Bone> armatureBones;
        std::unordered_map<std::string, unsigned int> armatureBoneIdMap;

        // node format should be that this node is the name of the armature
        // with only 1 child: the root bone of the actual bone heirarchy
        //assert(node->mNumChildren == 1);
        if (node->mNumChildren == 1)
        {
            // assuming every descendant node is a bone
            _extract_bones_from_node(node->mChildren[0], armatureBones, armatureBoneIdMap);
        }
        // otherwise we'll have to investigate how armatures are formatted

        // assume every descendant node is a bone?
        return std::make_shared<Armature>(armatureBones, armatureBoneIdMap);
    }

    
    void ModelManager::_extract_bones_from_node(const aiNode* node, std::vector<Bone>& armatureBones, std::unordered_map<std::string, unsigned int>& armatureBoneIdMap)
    {
        // add this bone to map
        unsigned int thisBoneId = static_cast<unsigned int>(armatureBones.size());
        // node name should be guarenteed to be valid for bones
        armatureBoneIdMap[node->mName.data] = thisBoneId;
        // add this node to armature
        armatureBones.push_back(
            Bone(node->mName.data, thisBoneId, assimp_converters::convert_matrix4(node->mTransformation))
        );
        // Bone will be missing inverse bind matrix, which is known by the submeshes

        // recurse
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            // we know what id our child will pick before we recurse into _process_node
            armatureBones[thisBoneId].childIds.push_back(static_cast<unsigned int>(armatureBones.size()));
            _extract_bones_from_node(node->mChildren[i], armatureBones, armatureBoneIdMap);
        }
    }
    
    void ModelManager::_process_supermesh(const aiScene *scene, const ImportReceipt& receipt)
    {
        if (receipt.supermeshName.empty()) return;

        // any meshes directly in root node?
        // scan assumes submeshes are only direct children of root node...
        std::string supermeshName = receipt.supermeshName;
        //PLEEPLOG_DEBUG("Loading supermesh " + supermeshName);
        
        m_supermeshMap[supermeshName] = _build_supermesh(scene, scene->mRootNode, receipt);
        m_supermeshMap[supermeshName]->m_name = supermeshName;
        m_supermeshMap[supermeshName]->m_sourceFilepath = receipt.importSourceFilepath;
    }
    
    std::shared_ptr<Supermesh> ModelManager::_build_supermesh(const aiScene* scene, const aiNode* node, const ImportReceipt& receipt) 
    {
        std::shared_ptr<Supermesh> newSupermesh = std::make_shared<Supermesh>();
        
        // process children
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            for (unsigned int m = 0; m < node->mChildren[i]->mNumMeshes; m++)
            {
                aiMesh* submesh = scene->mMeshes[node->mChildren[i]->mMeshes[m]];
                newSupermesh->m_submeshes.push_back(_build_mesh(submesh, receipt));
                // apply name outside of _build_mesh for ModelManagerFaux
                newSupermesh->m_submeshes.back()->m_name = std::string(submesh->mName.C_Str());
                // I don't know if both levels need to have the source, but might aswell
                newSupermesh->m_submeshes.back()->m_sourceFilepath = receipt.importSourceFilepath;
            }
        }

        return newSupermesh;
    }

    std::shared_ptr<Mesh> ModelManager::_build_mesh(const aiMesh *mesh, const ImportReceipt& receipt)
    {
        //PLEEPLOG_DEBUG("Loading mesh " + std::string(mesh->mName.C_Str()));
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // extract vertices
        _extract_vertices(vertices, mesh);
        // extract indices
        _extract_indices(indices, mesh);

        // extract bone weights
        // setup bone data for each vertex iterating by bones, not by vertices like above
        _extract_bone_weights_for_vertices(vertices, mesh, receipt);

        return std::make_shared<Mesh>(vertices, indices);
    }

    void ModelManager::_extract_vertices(std::vector<Vertex> &dest, const aiMesh *src)
    {
        for (unsigned int i = 0; i < src->mNumVertices; i++)
        {
            Vertex vertex;
            // we have to manually "cast" the assimp types
            // vertex coordinates
            vertex.position = assimp_converters::convert_vec3(src->mVertices[i]);

            // normals
            if (src->HasNormals())
            {
                vertex.normal = assimp_converters::convert_vec3(src->mNormals[i]);
            }

            // testure coordinates
            if (src->HasTextureCoords(0))
            {
                // Assuming we only use 1 set of texture coords (up to 8)
                vertex.texCoord.x = src->mTextureCoords[0][i].x;
                vertex.texCoord.y = src->mTextureCoords[0][i].y;

            }
            else
            {
                vertex.texCoord = glm::vec2(0.0f, 0.0f);
            }

            if (src->HasTangentsAndBitangents())
            {
                // tangent to face
                vertex.tangent = assimp_converters::convert_vec3(src->mTangents[i]);
            }

            // setup bone data as default (all disabled)
            vertex.set_bone_data_to_default();

            dest.push_back(vertex);
        }
    }

    void ModelManager::_extract_indices(std::vector<unsigned int> &dest, const aiMesh *src)
    {
        for (unsigned int i = 0; i < src->mNumFaces; i++)
        {
            aiFace tri = src->mFaces[i];
            for (unsigned int j = 0; j < tri.mNumIndices; j++)
                dest.push_back(tri.mIndices[j]);
        }
    }

    void ModelManager::_extract_bone_weights_for_vertices(std::vector<Vertex> &dest, const aiMesh *src, const ImportReceipt& receipt)
    {
        // parse through assimp bone list
        for (unsigned int boneIndex = 0; boneIndex < src->mNumBones; boneIndex++)
        {
            aiBone* boneData = src->mBones[boneIndex];
            std::string boneName = boneData->mName.C_Str();

            // find bone armature
            std::string armatureName = boneData->mArmature->mName.C_Str();
            // check if armature exists
            auto armatureIt = m_armatureMap.find(armatureName);
            // check if armature is part of THIS import
            auto armatureReceiptIt = std::find(receipt.armatureNames.begin(), receipt.armatureNames.end(), armatureName);
            // check if bone exists in imported armature
            auto boneIdIt = m_armatureMap[armatureName]->m_boneIdMap.find(boneName);
            // get bone id
            unsigned int boneId = boneIdIt->second;

            // while we're here, set mesh to bone transform in armature
            m_armatureMap[armatureName]->m_bones[boneId].m_bindTransform = assimp_converters::convert_matrix4(boneData->mOffsetMatrix);

            // now to actually set the weights...
            // fetch corresponding weight per vertex for *this* bone
            aiVertexWeight* vertexWeights = boneData->mWeights;

            for (unsigned int weightIndex = 0; weightIndex < boneData->mNumWeights; weightIndex++)
            {
                // indices in our vertex array SHOULD match ai indices
                unsigned int aiVertexId = vertexWeights[weightIndex].mVertexId;
                // this is an aiReal??
                float boneWeight = vertexWeights[weightIndex].mWeight;

                // check for weirdness
                assert(aiVertexId < dest.size());

                dest[aiVertexId].set_bone_data(boneId, boneWeight);
            }
        }
    }

    void ModelManager::_process_animations(const aiScene *scene, const ImportReceipt& receipt)
    {
        // load all animations from scene
        for (unsigned int i = 0; i < scene->mNumAnimations; i++)
        {
            aiAnimation *animation = scene->mAnimations[i];
            m_animationMap[std::string(animation->mName.C_Str())] = _build_animation(animation, receipt.animationNames[i]);

            // TODO: provide name, filepath, and ALL data required for serialization OUTSIDE of _build_animation, so that ModelManagerFaux's life is easy
        }
    }

    std::shared_ptr<AnimationSkeletal> ModelManager::_build_animation(const aiAnimation *animation, const std::string& animationName)
    {
        PLEEPLOG_WARN("NO IMPLEMENTATION FOR Loading animation: " + std::string(animation->mName.C_Str()));
        UNREFERENCED_PARAMETER(animation);
        UNREFERENCED_PARAMETER(animationName);

        // TODO: animations are weird...
        return nullptr;
    }
    
    std::shared_ptr<Supermesh> ModelManager::_build_cube_supermesh() 
    {
        // generate cube mesh data
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;
        
        // NOTE: tangent should be calculated based on normal and uv (texture coords)
        const float CUBE2_VERTICES[] = {
            // coordinates          // normal              // texture coords    // tangent
            -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    1.5f, -0.5f,         1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    1.5f,  1.5f,         1.0f,  0.0f,  0.0f,
             0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,   -0.5f,  1.5f,         1.0f,  0.0f,  0.0f,
             0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,   -0.5f, -0.5f,         1.0f,  0.0f,  0.0f,   // top

            -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    1.5f,  1.5f,         1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.5f, -0.5f,         1.0f,  0.0f,  0.0f,
             0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,   -0.5f, -0.5f,         1.0f,  0.0f,  0.0f,
             0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,   -0.5f,  1.5f,         1.0f,  0.0f,  0.0f,   // bottom

            -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    1.5f, -0.5f,         0.0f,  0.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,   -0.5f, -0.5f,         0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    1.5f,  1.5f,         0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,   -0.5f,  1.5f,         0.0f,  0.0f,  1.0f,   //left

            -0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.5f, -0.5f,         1.0f,  0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,   -0.5f, -0.5f,         1.0f,  0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.5f,  1.5f,         1.0f,  0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,   -0.5f,  1.5f,         1.0f,  0.0f, 0.0f,   // front

             0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    1.5f, -0.5f,         0.0f,  0.0f, -1.0f,
             0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,   -0.5f, -0.5f,         0.0f,  0.0f, -1.0f,
             0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    1.5f,  1.5f,         0.0f,  0.0f, -1.0f,
             0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,   -0.5f,  1.5f,         0.0f,  0.0f, -1.0f,   // right

            -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,   -0.5f, -0.5f,        -1.0f,  0.0f, 0.0f,
             0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.5f, -0.5f,        -1.0f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,   -0.5f,  1.5f,        -1.0f,  0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.5f,  1.5f,        -1.0f,  0.0f, 0.0f,   // back
        };
        // hardcode 3+3+2+3
        for (unsigned int i = 0; i < sizeof(CUBE2_VERTICES) / sizeof(float) / 11; i++)
        {
            vertices.push_back( Vertex{
                glm::vec3(CUBE2_VERTICES[i * 11 + 0], CUBE2_VERTICES[i * 11 + 1], CUBE2_VERTICES[i * 11 + 2]), 
                glm::vec3(CUBE2_VERTICES[i * 11 + 3], CUBE2_VERTICES[i * 11 + 4], CUBE2_VERTICES[i * 11 + 5]), 
                glm::vec2(CUBE2_VERTICES[i * 11 + 6], CUBE2_VERTICES[i * 11 + 7]), 
                glm::vec3(CUBE2_VERTICES[i * 11 + 8], CUBE2_VERTICES[i * 11 + 9], CUBE2_VERTICES[i * 11 +10])
            } );

            // Don't forget Bones!
            vertices.back().set_bone_data_to_default();
        }

        unsigned int CUBE2_INDICES[] = {
            0,1,2,
            0,2,3,

            4,6,5,
            4,7,6,

            8,10,9,
            9,10,11,

            12,14,13,
            13,14,15,

            16,18,17,
            17,18,19,

            20,21,23,
            20,23,22
        };
        for (unsigned int i = 0; i < sizeof(CUBE2_INDICES) / sizeof(unsigned int); i++)
        {
            indices.push_back(CUBE2_INDICES[i]);
        }

        return std::make_shared<Supermesh>(
            ENUM_TO_STR(BasicSupermeshType::cube),
            ENUM_TO_STR(BasicSupermeshType::cube),
            std::make_shared<Mesh>(vertices, indices)
        );
    }
    
    std::shared_ptr<Supermesh> ModelManager::_build_quad_supermesh() 
    {
        // generate quad mesh data
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;
        
        const float QUAD_VERTICES[] = {
            // coordinates          // normal              // texture coords    // tangent
            -0.5f,  0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    0.0f,  0.0f,         1.0f,  0.0f,  0.0f,
             0.5f,  0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    1.0f,  0.0f,         1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    0.0f,  1.0f,         1.0f,  0.0f,  0.0f,
             0.5f, -0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    1.0f,  1.0f,         1.0f,  0.0f,  0.0f
        };
        // hardcode 3+3+2
        for (unsigned int i = 0; i < sizeof(QUAD_VERTICES) / sizeof(float) / 11; i++)
        {
            vertices.push_back( Vertex{
                glm::vec3(QUAD_VERTICES[i * 11 + 0], QUAD_VERTICES[i * 11 + 1], QUAD_VERTICES[i * 11 + 2]), 
                glm::vec3(QUAD_VERTICES[i * 11 + 3], QUAD_VERTICES[i * 11 + 4], QUAD_VERTICES[i * 11 + 5]), 
                glm::vec2(QUAD_VERTICES[i * 11 + 6], QUAD_VERTICES[i * 11 + 7]),
                glm::vec3(QUAD_VERTICES[i * 11 + 8], QUAD_VERTICES[i * 11 + 9], QUAD_VERTICES[i * 11 +10])
            } );
            
            // Don't forget Bones!
            vertices.back().set_bone_data_to_default();
        }

        const unsigned int QUAD_INDICES[] = {
            0,2,1,
            1,2,3,
        };
        for (unsigned int i = 0; i < sizeof(QUAD_INDICES) / sizeof(unsigned int); i++)
        {
            indices.push_back(QUAD_INDICES[i]);
        }

        return std::make_shared<Supermesh>(
            ENUM_TO_STR(BasicSupermeshType::quad),
            ENUM_TO_STR(BasicSupermeshType::quad),
            std::make_shared<Mesh>(vertices, indices)
        );
    }
    
    std::shared_ptr<Supermesh> ModelManager::_build_screen_supermesh() 
    {
        // generate screen plane mesh data
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;

        const float SCREEN_VERTICES[] = {
            // coordinates          // texture coords
            -1.0f,  1.0f,  0.0f,    0.0f, 1.0f,
             1.0f,  1.0f,  0.0f,    1.0f, 1.0f,
            -1.0f, -1.0f,  0.0f,    0.0f, 0.0f,
             1.0f, -1.0f,  0.0f,    1.0f, 0.0f
        };
        // hardcode attrib 3+2
        for (unsigned int i = 0; i < sizeof(SCREEN_VERTICES) / sizeof(float) / 5; i++)
        {
            vertices.push_back( Vertex{
                glm::vec3(SCREEN_VERTICES[i * 5 + 0], SCREEN_VERTICES[i * 5 + 1], SCREEN_VERTICES[i * 5 + 2]), 
                glm::vec3(0.0f), // screen shaders should not use normals 
                glm::vec2(SCREEN_VERTICES[i * 5 + 3], SCREEN_VERTICES[i * 5 + 4]),
                glm::vec3(0.0f) // screen shaders should not use tangents
            } );

            // Don't forget Bones!
            vertices.back().set_bone_data_to_default();
        }

        const unsigned int SCREEN_INDICES[] = {
            0,2,1,
            1,2,3
        };
        for (unsigned int i = 0; i < sizeof(SCREEN_INDICES) / sizeof(unsigned int); i++)
        {
            indices.push_back(SCREEN_INDICES[i]);
        }

        return std::make_shared<Supermesh>(
            ENUM_TO_STR(BasicSupermeshType::screen),
            ENUM_TO_STR(BasicSupermeshType::screen),
            std::make_shared<Mesh>(vertices, indices)
        );
    }
    
    std::shared_ptr<Supermesh> ModelManager::_build_icosahedron_supermesh() 
    {
        /*
        The vertices of an icosahedron centered at the origin with 
        an edge length of 2 and a circumradius of sqrt(phi^2 + 1) (~1.902) are:
        {
            ( 0,   ±1,   ±phi),
            (±1,   ±phi,  0)
            (±phi,  0,   ±1)
        }
        */
        // convert from desired radius (0.5) to default radius from construction algorithm
        const float c = 0.5f / 1.902f;

        // generate quad mesh data
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;

        // (1.0f + glm::sqrt(5.0f)) / 2.0f
        const float phi = glm::golden_ratio<float>();

        // TODO: replace interpolated normals with face normals (requires 3 verts for all 20 faces)
        
        const float ICOSA_VERTICES[] = {
            // coordinates          // normal              // texture coords    // tangent
             0.0f,  1.0f,   phi,    0.0f,  1.0f,   phi,    0.5f,  0.0f,         1.0f,  0.0f,  0.0f,
             0.0f, -1.0f,   phi,    0.0f, -1.0f,   phi,    0.5f,  0.0f,        -1.0f,  0.0f,  0.0f,
             0.0f, -1.0f,  -phi,    0.0f, -1.0f,  -phi,    0.5f,  0.0f,         1.0f,  0.0f,  0.0f,
             0.0f,  1.0f,  -phi,    0.0f,  1.0f,  -phi,    0.5f,  0.0f,        -1.0f,  0.0f,  0.0f,

             1.0f,  phi,   0.0f,    1.0f,  phi,   0.0f,    1.0f,  0.5f,         0.0f,  0.0f,  1.0f,
            -1.0f,  phi,   0.0f,   -1.0f,  phi,   0.0f,    1.0f,  0.5f,         0.0f,  0.0f, -1.0f,
            -1.0f, -phi,   0.0f,   -1.0f, -phi,   0.0f,    1.0f,  0.5f,         0.0f,  0.0f,  1.0f,
             1.0f, -phi,   0.0f,    1.0f, -phi,   0.0f,    1.0f,  0.5f,         0.0f,  0.0f, -1.0f,

             phi,   0.0f,  1.0f,    phi,   0.0f,  1.0f,    0.0f,  1.0f,         0.0f,  1.0f,  0.0f,
            -phi,   0.0f,  1.0f,   -phi,   0.0f,  1.0f,    0.0f,  1.0f,         0.0f, -1.0f,  0.0f,
            -phi,   0.0f, -1.0f,   -phi,   0.0f, -1.0f,    0.0f,  1.0f,         0.0f,  1.0f,  0.0f,
             phi,   0.0f, -1.0f,    phi,   0.0f, -1.0f,    0.0f,  1.0f,         0.0f, -1.0f,  0.0f
        };
        // hardcode 3+3+2
        for (unsigned int i = 0; i < sizeof(ICOSA_VERTICES) / sizeof(float) / 11; i++)
        {
            vertices.push_back( Vertex{
                glm::vec3(ICOSA_VERTICES[i * 11 + 0]*c, ICOSA_VERTICES[i * 11 + 1]*c, ICOSA_VERTICES[i * 11 + 2]*c), 
                glm::vec3(ICOSA_VERTICES[i * 11 + 3], ICOSA_VERTICES[i * 11 + 4], ICOSA_VERTICES[i * 11 + 5]), 
                glm::vec2(ICOSA_VERTICES[i * 11 + 6], ICOSA_VERTICES[i * 11 + 7]),
                glm::vec3(ICOSA_VERTICES[i * 11 + 8], ICOSA_VERTICES[i * 11 + 9], ICOSA_VERTICES[i * 11 +10])
            } );
            
            // Don't forget Bones!
            vertices.back().set_bone_data_to_default();
        }

        const unsigned int ICOSA_INDICES[] = {
            0,4,5,      // top pyramid
            0,8,4,
            0,1,8,
            0,9,1,
            0,5,9,

            1,6,7,      // middle strip
            1,9,6,
            9,10,6,
            9,5,10,
            5,3,10,
            5,4,3,
            4,11,3,
            4,8,11,
            8,7,11,
            8,1,7,

            2,3,11,   // bottom pyramid
            2,11,7,
            2,7,6,
            2,6,10,
            2,10,3
        };
        for (unsigned int i = 0; i < sizeof(ICOSA_INDICES) / sizeof(unsigned int); i++)
        {
            indices.push_back(ICOSA_INDICES[i]);
        }

        return std::make_shared<Supermesh>(
            ENUM_TO_STR(BasicSupermeshType::icosahedron),
            ENUM_TO_STR(BasicSupermeshType::icosahedron),
            std::make_shared<Mesh>(vertices, indices)
        );
    }
    
    std::shared_ptr<Supermesh> ModelManager::_build_vector_supermesh()
    {
        // generate pyramid mesh data
        std::vector<Vertex>       vertices;
        std::vector<unsigned int> indices;
        
        // NOTE: tangent should be calculated based on normal and uv (texture coords)
        const float VECTOR_VERTICES[] = {
            // coordinates          // normal              // texture coords    // tangent
             0.0f, 0.0f, 1.0f,       0.0f, 0.0f, 1.0f,      0.0f, 0.0f,          1.0f, 0.0f, 0.0f,   // point

             0.1f, -0.1f, 0.0f,      1.0f, -1.0f, 0.0f,     1.0f, 0.0f,          0.0f, 0.0f, -1.0f,
             0.1f,  0.1f, 0.0f,      1.0f,  1.0f, 0.0f,     1.0f, 1.0f,          0.0f, 0.0f, -1.0f,
            -0.1f, -0.1f, 0.0f,     -1.0f, -1.0f, 0.0f,     1.0f, 1.0f,          0.0f, 0.0f, -1.0f,
            -0.1f,  0.1f, 0.0f,     -1.0f,  1.0f, 0.0f,     0.0f, 1.0f,          0.0f, 0.0f, -1.0f  // base
        };
        // hardcode 3+3+2+3
        for (unsigned int i = 0; i < sizeof(VECTOR_VERTICES) / sizeof(float) / 11; i++)
        {
            vertices.push_back( Vertex{
                glm::vec3(VECTOR_VERTICES[i * 11 + 0], VECTOR_VERTICES[i * 11 + 1], VECTOR_VERTICES[i * 11 + 2]), 
                glm::vec3(VECTOR_VERTICES[i * 11 + 3], VECTOR_VERTICES[i * 11 + 4], VECTOR_VERTICES[i * 11 + 5]), 
                glm::vec2(VECTOR_VERTICES[i * 11 + 6], VECTOR_VERTICES[i * 11 + 7]), 
                glm::vec3(VECTOR_VERTICES[i * 11 + 8], VECTOR_VERTICES[i * 11 + 9], VECTOR_VERTICES[i * 11 +10])
            } );

            // Don't forget Bones!
            vertices.back().set_bone_data_to_default();
        }

        unsigned int VECTOR_INDICES[] = {
            0,1,2,
            0,2,4,
            0,4,3,
            0,3,1
        };
        for (unsigned int i = 0; i < sizeof(VECTOR_INDICES) / sizeof(unsigned int); i++)
        {
            indices.push_back(VECTOR_INDICES[i]);
        }

        return std::make_shared<Supermesh>(
            ENUM_TO_STR(BasicSupermeshType::vector),
            ENUM_TO_STR(BasicSupermeshType::vector),
            std::make_shared<Mesh>(vertices, indices)
        );
    }

    void ModelManager::debug_scene(const aiScene *scene)
    {
        PLEEPLOG_DEBUG("Scene name: " + std::string(scene->mName.C_Str()));
        PLEEPLOG_DEBUG("    meshes: " + std::to_string(scene->mNumMeshes));
        PLEEPLOG_DEBUG("    materials: " + std::to_string(scene->mNumMaterials));
        PLEEPLOG_DEBUG("    textures: " + std::to_string(scene->mNumTextures));
        PLEEPLOG_DEBUG("    animations: " + std::to_string(scene->mNumAnimations));
        
        debug_nodes(scene, scene->mRootNode);
    }

    void ModelManager::debug_nodes(const aiScene *scene, aiNode *node, const unsigned int depth)
    {
        PLEEPLOG_DEBUG("Node name: " + std::string(node->mName.C_Str()));
        PLEEPLOG_DEBUG("meshes:");
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *nodeMesh = scene->mMeshes[node->mMeshes[i]];
            PLEEPLOG_DEBUG("    Mesh name: " + std::string(nodeMesh->mName.C_Str()));
            PLEEPLOG_DEBUG("    Mesh Material: " + std::string(scene->mMaterials[nodeMesh->mMaterialIndex]->GetName().C_Str()));
            PLEEPLOG_DEBUG("    Mesh vertices: " + std::to_string(nodeMesh->mNumVertices));
            PLEEPLOG_DEBUG("    Mesh faces: " + std::to_string(nodeMesh->mNumFaces));
            PLEEPLOG_DEBUG("    Mesh animMeshes: " + std::to_string(nodeMesh->mNumAnimMeshes));
            PLEEPLOG_DEBUG("    Mesh bones: " + std::to_string(nodeMesh->mNumBones));
            for (unsigned int b = nodeMesh->mNumBones - 1; b < nodeMesh->mNumBones; b--)
            {
                PLEEPLOG_DEBUG("        " + std::string(nodeMesh->mBones[b]->mName.C_Str()) 
                    + " from node " 
                    + std::string(nodeMesh->mBones[b]->mNode ? nodeMesh->mBones[b]->mNode->mName.C_Str() : "NULL") 
                    + " and armature " 
                    + std::string(nodeMesh->mBones[b]->mArmature ? nodeMesh->mBones[b]->mArmature->mName.C_Str() : "NULL"));
            }
        }
        PLEEPLOG_DEBUG("I have " + std::to_string(node->mNumChildren) + " children:");
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            PLEEPLOG_DEBUG("    " + std::string(node->mChildren[i]->mName.C_Str()));
        }
        PLEEPLOG_DEBUG(std::string(depth, '*'));

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            debug_nodes(scene, node->mChildren[i], depth + 1);
        }
    }
    
    void ModelManager::debug_receipt(const ImportReceipt& receipt) 
    {
        PLEEPLOG_DEBUG("Import receipt: " + receipt.importName);
        PLEEPLOG_DEBUG("from file: " + receipt.importSourceFilepath);
        PLEEPLOG_DEBUG("Supermesh name: " + receipt.supermeshName);
        PLEEPLOG_DEBUG("Supermesh submesh names:");
        for (auto name : receipt.supermeshSubmeshNames)
        {
            PLEEPLOG_DEBUG("    " + name);
        }
        PLEEPLOG_DEBUG("Supermesh Material names:");
        for (auto name : receipt.supermeshMaterialNames)
        {
            PLEEPLOG_DEBUG("    " + name);
        }
        PLEEPLOG_DEBUG("All Material names:");
        for (auto name : receipt.materialNames)
        {
            PLEEPLOG_DEBUG("    " + name);
        }
        PLEEPLOG_DEBUG("Armature names:");
        for (auto name : receipt.armatureNames)
        {
            PLEEPLOG_DEBUG("    " + name);
        }
        PLEEPLOG_DEBUG("Animation names:");
        for (auto name : receipt.animationNames)
        {
            PLEEPLOG_DEBUG("    " + name);
        }
    }
}