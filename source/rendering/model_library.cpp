#include "rendering/model_library.h"

#include <cassert>

#include "logging/pleep_log.h"
#include "rendering/assimp_converters.h"

namespace pleep
{
    // Create static library instance immediately
    // we must call constructor ourselves (it is protected)
    // but we pass ownership to unique_ptr, so no delete needed
    std::unique_ptr<ModelLibrary> ModelLibrary::m_singleton(new ModelLibrary);

    ModelLibrary::ImportReceipt ModelLibrary::import(std::string filepath)
    {
        // check for hardcoded assets
        if (filepath == ModelLibrary::m_singleton->cubeKey)
        {
            ModelLibrary::m_singleton->m_supermeshMap[filepath] = ModelLibrary::m_singleton->_build_cube_supermesh();
            return ImportReceipt{filepath, filepath, {filepath}};
        }
        else if (filepath == ModelLibrary::m_singleton->quadKey)
        {
            ModelLibrary::m_singleton->m_supermeshMap[filepath] = ModelLibrary::m_singleton->_build_quad_supermesh();
            return ImportReceipt{filepath, filepath, {filepath}};
        }
        else if (filepath == ModelLibrary::m_singleton->screenKey)
        {
            ModelLibrary::m_singleton->m_supermeshMap[filepath] = ModelLibrary::m_singleton->_build_screen_supermesh();
            return ImportReceipt{filepath, filepath, {filepath}};
        }
        else if (filepath == ModelLibrary::m_singleton->icosahedronKey)
        {
            ModelLibrary::m_singleton->m_supermeshMap[filepath] = ModelLibrary::m_singleton->_build_icosahedron_supermesh();
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
        //_debug_scene(scene);
        //return ImportReceipt{};
        // ***********************************

        // check for possible assets and load into receipt
        ImportReceipt receipt = ModelLibrary::m_singleton->_scan_scene(scene, filestem);
        receipt.importSourceFilepath = filepath;
        
        // process materials and armatures first for meshes to reference
        ModelLibrary::m_singleton->_process_materials(scene, receipt, directory);
        ModelLibrary::m_singleton->_process_armatures(scene, receipt);
        ModelLibrary::m_singleton->_process_supermeshes(scene, receipt);
        ModelLibrary::m_singleton->_process_animations(scene, receipt);

        _debug_receipt(receipt);
        // now all assets are cached, user can call fetch methods using receipt names
        return receipt;
    }

    bool ModelLibrary::create_material(const std::string& name, const std::unordered_map<TextureType, std::string>& textureDict) 
    {
        auto materialIt = ModelLibrary::m_singleton->m_materialMap.find(name);
        if (materialIt != ModelLibrary::m_singleton->m_materialMap.end())
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
        ModelLibrary::m_singleton->m_materialMap[name] = newMat;
        return true;
    }

    std::shared_ptr<const Supermesh> ModelLibrary::fetch_supermesh(const std::string& name)
    {
        auto meshIt = ModelLibrary::m_singleton->m_supermeshMap.find(name);
        if (meshIt == ModelLibrary::m_singleton->m_supermeshMap.end())
        {
            PLEEPLOG_WARN("Supermesh " + name + " is not cached.");
            return nullptr;
        }
        else
        {
            return meshIt->second;
        }
    }

    std::shared_ptr<const Material> ModelLibrary::fetch_material(const std::string& name)
    {
        auto materialIt = ModelLibrary::m_singleton->m_materialMap.find(name);
        if (materialIt == ModelLibrary::m_singleton->m_materialMap.end())
        {
            PLEEPLOG_WARN("Material " + name + " is not cached.");
            return nullptr;
        }
        else
        {
            return materialIt->second;
        }
    }

    std::shared_ptr<Armature> ModelLibrary::fetch_armature(const std::string& name)
    {
        auto armatureIt = ModelLibrary::m_singleton->m_armatureMap.find(name);
        if (armatureIt == ModelLibrary::m_singleton->m_armatureMap.end())
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

    std::shared_ptr<const AnimationSkeletal> ModelLibrary::fetch_animation(const std::string& name)
    {
        auto animationIt = ModelLibrary::m_singleton->m_animationMap.find(name);
        if (animationIt == ModelLibrary::m_singleton->m_animationMap.end())
        {
            PLEEPLOG_WARN("Material " + name + " is not cached.");
            return nullptr;
        }
        else
        {
            return animationIt->second;
        }
    }
    
    std::shared_ptr<const Supermesh> ModelLibrary::fetch_cube_supermesh() 
    {
        // inline "fetch_supermesh(cubeKey)"
        auto meshIt = ModelLibrary::m_singleton->m_supermeshMap.find(ModelLibrary::m_singleton->cubeKey);
        if (meshIt != ModelLibrary::m_singleton->m_supermeshMap.end())
        {
            return meshIt->second;
        }

        ModelLibrary::m_singleton->import(ModelLibrary::m_singleton->cubeKey);
        return ModelLibrary::m_singleton->m_supermeshMap[ModelLibrary::m_singleton->cubeKey];
    }
    
    std::shared_ptr<const Supermesh> ModelLibrary::fetch_quad_supermesh() 
    {
        // inline "fetch_supermesh(quadKey)"
        auto meshIt = ModelLibrary::m_singleton->m_supermeshMap.find(ModelLibrary::m_singleton->quadKey);
        if (meshIt != ModelLibrary::m_singleton->m_supermeshMap.end())
        {
            return meshIt->second;
        }

        ModelLibrary::m_singleton->import(ModelLibrary::m_singleton->quadKey);
        return ModelLibrary::m_singleton->m_supermeshMap[ModelLibrary::m_singleton->quadKey];
    }
    
    std::shared_ptr<const Supermesh> ModelLibrary::fetch_screen_supermesh() 
    {
        // inline "fetch_supermesh(screenKey)"
        auto meshIt = ModelLibrary::m_singleton->m_supermeshMap.find(ModelLibrary::m_singleton->screenKey);
        if (meshIt != ModelLibrary::m_singleton->m_supermeshMap.end())
        {
            return meshIt->second;
        }

        ModelLibrary::m_singleton->import(ModelLibrary::m_singleton->screenKey);
        return ModelLibrary::m_singleton->m_supermeshMap[ModelLibrary::m_singleton->screenKey];
    }
    
    std::shared_ptr<const Supermesh> ModelLibrary::fetch_icosahedron_supermesh() 
    {
        // inline "fetch_supermesh(screenKey)"
        auto meshIt = ModelLibrary::m_singleton->m_supermeshMap.find(ModelLibrary::m_singleton->icosahedronKey);
        if (meshIt != ModelLibrary::m_singleton->m_supermeshMap.end())
        {
            return meshIt->second;
        }

        ModelLibrary::m_singleton->import(ModelLibrary::m_singleton->icosahedronKey);
        return ModelLibrary::m_singleton->m_supermeshMap[ModelLibrary::m_singleton->icosahedronKey];
    }

    void ModelLibrary::clear_unused()
    {
        PLEEPLOG_WARN("NO IMPLEMENTATION!");
        // Classic removing from dynamic container problem
        /*
                ModelLibrary::m_singleton->m_meshMap.erase(
                    std::remove_if(
                        ModelLibrary::m_singleton->m_meshMap.begin(),
                        ModelLibrary::m_singleton->m_meshMap.end(),
                        [](std::pair<std::string, std::shared_ptr<const Mesh>> meshIt) {
                            if (meshIt.second.use_count() > 1) return false;
                            return true;
                        }
                    ),
                    ModelLibrary::m_singleton->m_meshMap.end()
                );

                ModelLibrary::m_singleton->m_materialMap.erase(
                    std::remove_if(
                        ModelLibrary::m_singleton->m_materialMap.begin(),
                        ModelLibrary::m_singleton->m_materialMap.end(),
                        [](std::pair<std::string, std::shared_ptr<const Material>> materialIt) {
                            if (materialIt.second.use_count() > 1) return false;
                            return true;
                        }
                    ),
                    ModelLibrary::m_singleton->m_materialMap.end()
                );

                ModelLibrary::m_singleton->m_armatureMap.erase(
                    std::remove_if(
                        ModelLibrary::m_singleton->m_armatureMap.begin(),
                        ModelLibrary::m_singleton->m_armatureMap.end(),
                        [](std::pair<std::string, std::shared_ptr<Armature>> armatureIt) {
                            if (armatureIt.second.use_count() > 1) return false;
                            return true;
                        }
                    ),
                    ModelLibrary::m_singleton->m_armatureMap.end()
                );

                ModelLibrary::m_singleton->m_animationMap.erase(
                    std::remove_if(
                        ModelLibrary::m_singleton->m_animationMap.begin(),
                        ModelLibrary::m_singleton->m_animationMap.end(),
                        [](std::pair<std::string, std::shared_ptr<const AnimationSkeletal>> animationIt) {
                            if (animationIt.second.use_count() > 1) return false;
                            return true;
                        }
                    ),
                    ModelLibrary::m_singleton->m_animationMap.end()
                );
         */
    }

    void ModelLibrary::clear_library()
    {
        ModelLibrary::m_singleton->m_supermeshMap.clear();
        ModelLibrary::m_singleton->m_materialMap.clear();
        ModelLibrary::m_singleton->m_armatureMap.clear();
        ModelLibrary::m_singleton->m_animationMap.clear();
    }

    void ModelLibrary::set_vertex_bone_data_to_default(Vertex &vertex)
    {
        for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            vertex.boneIds[i] = -1;
            vertex.boneWeights[i] = 0.0f;
        }
    }

    void ModelLibrary::set_vertex_bone_data(Vertex &vertex, int boneId, float weight)
    {
        for (unsigned int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            if (vertex.boneIds[i] < 0)
            {
                vertex.boneWeights[i] = weight;
                vertex.boneIds[i] = boneId;
                break;
            }
        }
    }
    
    ModelLibrary::ImportReceipt ModelLibrary::_scan_scene(const aiScene* scene, const std::string& nameDefault) 
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
            if (m_materialMap.find(matName) != m_materialMap.end())
            {
                PLEEPLOG_WARN("Could not import " + matName + " it already exists in the library");
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
            if (m_animationMap.find(animName) != m_animationMap.end())
            {
                PLEEPLOG_WARN("Could not import " + animName + " it already exists in the library");
                // we could try to generate a default name here? unless this already is the deafult name
                continue;
            }

            receipt.animationNames.push_back(animName);
        }

        // can scene not have root node?
        // scene.h::255 > "There will always be at least the root node if the import was successful"
        assert(scene->mRootNode);
        
        // Assume supermeshes are children of root node who have >1 meshes
        // Assume armatures are children of root node who have <1 meshes
        // TODO: name may be blank? how to give unique name?
        for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++)
        {
            if (scene->mRootNode->mChildren[i]->mNumMeshes > 0)
            {
                std::string supermeshName = scene->mRootNode->mChildren[i]->mName.C_Str();
                if (supermeshName.empty())
                {
                    supermeshName = receipt.importName + "_supermesh_" + std::to_string(i);
                }
                // check uniqueness
                if (m_supermeshMap.find(supermeshName) != m_supermeshMap.end())
                {
                    PLEEPLOG_WARN("Could not import " + supermeshName + " it already exists in the library");
                    // we could try to generate a default name here? unless this already is the deafult name
                    continue;
                }
                receipt.supermeshNames.push_back(supermeshName);
            }
            else
            {
                std::string armatureName = scene->mRootNode->mChildren[i]->mName.C_Str();
                if (armatureName.empty())
                {
                    armatureName = receipt.importName + "_armature_" + std::to_string(i);
                }
                // check uniqueness
                if (m_armatureMap.find(armatureName) != m_armatureMap.end())
                {
                    PLEEPLOG_WARN("Could not import " + armatureName + " it already exists in the library");
                    // we could try to generate a default name here? unless this already is the deafult name
                    continue;
                }
                receipt.armatureNames.push_back(armatureName);
            }
        }

        return receipt;
    }

    
    void ModelLibrary::_process_materials(const aiScene *scene, ImportReceipt& receipt, const std::string directory)
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
            m_materialMap[materialName]->m_sourceFilename = receipt.importSourceFilepath;
        }
    }

    std::shared_ptr<Material> ModelLibrary::_build_material(aiMaterial *material, const std::string& materialName, const std::string& directory)
    {
        //PLEEPLOG_DEBUG("Loading material: " + std::string(materialName));

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

        std::shared_ptr<Material> newMaterial = std::make_shared<Material>(std::move(loadedTextures));
        newMaterial->m_name = materialName;
        return newMaterial;
    }

    void ModelLibrary::_process_armatures(const aiScene *scene, ImportReceipt& receipt)
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
            m_armatureMap[armatureName]->m_sourceFilepath = receipt.importSourceFilepath;
        }
    }

    std::shared_ptr<Armature> ModelLibrary::_build_armature(aiNode* node, const std::string& armatureName)
    {
        //PLEEPLOG_DEBUG("Loading armature: " + std::string(armatureName));
        std::vector<Bone> armatureBones;
        std::unordered_map<std::string, unsigned int> armatureBoneIdMap;

        // node format should be that this node is the name of the armature
        // with only 1 child: the root bone of the actual bone heirarchy
        assert(node->mNumChildren == 1);
        // assuming every descendant node is a bone
        _extract_bones_from_node(node->mChildren[0], armatureBones, armatureBoneIdMap);

        // assume every descendant node is a bone?
        std::shared_ptr<Armature> newArmature = std::make_shared<Armature>(armatureBones, armatureBoneIdMap);
        newArmature->m_name = armatureName;
        return newArmature;
    }

    
    void ModelLibrary::_extract_bones_from_node(aiNode* node, std::vector<Bone>& armatureBones, std::unordered_map<std::string, unsigned int>& armatureBoneIdMap)
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
    
    void ModelLibrary::_process_supermeshes(const aiScene *scene, ImportReceipt& receipt)
    {
        for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++)
        {
            if (scene->mRootNode->mChildren[i]->mNumMeshes <= 0) continue;

            std::string supermeshName = scene->mRootNode->mChildren[i]->mName.C_Str();
            if (supermeshName.empty())
            {
                supermeshName = receipt.importName + "_supermesh_" + std::to_string(i);
            }
            // check if key was validated by scan
            if (std::find(receipt.supermeshNames.begin(), receipt.supermeshNames.end(), supermeshName) == receipt.supermeshNames.end()) continue;

            // inline _build_supermesh()
            std::shared_ptr<Supermesh> newSupermesh = std::make_shared<Supermesh>();
            newSupermesh->m_name = supermeshName;
            newSupermesh->m_sourceFilepath = receipt.importSourceFilepath;
            // emplace empty submesh material names list for this supermesh
            receipt.supermeshMaterialsNames.push_back({});
            for (unsigned int m = 0; m < scene->mRootNode->mChildren[i]->mNumMeshes; m++)
            {
                aiMesh* submesh = scene->mMeshes[scene->mRootNode->mChildren[i]->mMeshes[m]];
                newSupermesh->m_submeshes.push_back(_build_mesh(submesh, receipt));
                // I don't know if both levels need to have the source, but might aswell
                newSupermesh->m_submeshes.back()->m_sourceFilename = receipt.importSourceFilepath;

                // Associate a material with this submesh in the receipt for this supermesh
                aiMaterial* material = scene->mMaterials[submesh->mMaterialIndex];
                receipt.supermeshMaterialsNames.back().push_back(material->GetName().C_Str());
            }

            // should be validated by scene scan to not override
            m_supermeshMap[supermeshName] = newSupermesh;
        }
    }

    std::shared_ptr<Mesh> ModelLibrary::_build_mesh(aiMesh *mesh, ImportReceipt& receipt)
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

    void ModelLibrary::_extract_vertices(std::vector<Vertex> &dest, aiMesh *src)
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
            if (src->mTextureCoords[0])
            {
                // Assuming we only use 1 set of texture coords (up to 8)
                vertex.texCoord.x = src->mTextureCoords[0][i].x;
                vertex.texCoord.y = src->mTextureCoords[0][i].y;

                // tangent to face
                vertex.tangent = assimp_converters::convert_vec3(src->mTangents[i]);
            }
            else
            {
                vertex.texCoord = glm::vec2(0.0f, 0.0f);
            }

            // setup bone data as default (all disabled)
            ModelLibrary::set_vertex_bone_data_to_default(vertex);

            dest.push_back(vertex);
        }
    }

    void ModelLibrary::_extract_indices(std::vector<unsigned int> &dest, aiMesh *src)
    {
        for (unsigned int i = 0; i < src->mNumFaces; i++)
        {
            aiFace tri = src->mFaces[i];
            for (unsigned int j = 0; j < tri.mNumIndices; j++)
                dest.push_back(tri.mIndices[j]);
        }
    }

    void ModelLibrary::_extract_bone_weights_for_vertices(std::vector<Vertex> &dest, aiMesh *src, ImportReceipt& receipt)
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

                ModelLibrary::set_vertex_bone_data(dest[aiVertexId], boneId, boneWeight);
            }
        }
    }

    void ModelLibrary::_process_animations(const aiScene *scene, ImportReceipt& receipt)
    {
        // load all animations from scene
        for (unsigned int i = 0; i < scene->mNumAnimations; i++)
        {
            aiAnimation *animation = scene->mAnimations[i];
            m_animationMap[std::string(animation->mName.C_Str())] = _build_animation(animation, receipt.animationNames[i]);
        }
    }

    std::shared_ptr<AnimationSkeletal> ModelLibrary::_build_animation(aiAnimation *animation, const std::string& animationName)
    {
        PLEEPLOG_WARN("NO IMPLEMENTATION FOR Loading animation: " + std::string(animation->mName.C_Str()));
        UNREFERENCED_PARAMETER(animation);
        UNREFERENCED_PARAMETER(animationName);

        // TODO: animations are weird...
        return nullptr;
    }
    
    std::shared_ptr<Supermesh> ModelLibrary::_build_cube_supermesh() 
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
            ModelLibrary::set_vertex_bone_data_to_default(vertices.back());
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
            cubeKey,
            cubeKey,
            std::make_shared<Mesh>(vertices, indices)
        );
    }
    
    std::shared_ptr<Supermesh> ModelLibrary::_build_quad_supermesh() 
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
            ModelLibrary::set_vertex_bone_data_to_default(vertices.back());
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
            quadKey,
            quadKey,
            std::make_shared<Mesh>(vertices, indices)
        );
    }
    
    std::shared_ptr<Supermesh> ModelLibrary::_build_screen_supermesh() 
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
            ModelLibrary::set_vertex_bone_data_to_default(vertices.back());
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
            screenKey,
            screenKey,
            std::make_shared<Mesh>(vertices, indices)
        );
    }
    
    std::shared_ptr<Supermesh> ModelLibrary::_build_icosahedron_supermesh() 
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
            ModelLibrary::set_vertex_bone_data_to_default(vertices.back());
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
            icosahedronKey,
            icosahedronKey,
            std::make_shared<Mesh>(vertices, indices)
        );
    }
    

    void ModelLibrary::_debug_scene(const aiScene *scene)
    {
        PLEEPLOG_DEBUG("Scene name: " + std::string(scene->mName.C_Str()));
        PLEEPLOG_DEBUG("    meshes: " + std::to_string(scene->mNumMeshes));
        PLEEPLOG_DEBUG("    materials: " + std::to_string(scene->mNumMaterials));
        PLEEPLOG_DEBUG("    textures: " + std::to_string(scene->mNumTextures));
        PLEEPLOG_DEBUG("    animations: " + std::to_string(scene->mNumAnimations));
        
        _debug_nodes(scene, scene->mRootNode);
    }

    void ModelLibrary::_debug_nodes(const aiScene *scene, aiNode *node, const unsigned int depth)
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
            _debug_nodes(scene, node->mChildren[i], depth + 1);
        }
    }
    
    void ModelLibrary::_debug_receipt(const ImportReceipt& receipt) 
    {
        PLEEPLOG_DEBUG("Import receipt: " + receipt.importName);
        PLEEPLOG_DEBUG("from file: " + receipt.importSourceFilepath);
        PLEEPLOG_DEBUG("Supermesh names:");
        for (auto name : receipt.supermeshNames)
        {
            PLEEPLOG_DEBUG("    " + name);
        }
        PLEEPLOG_DEBUG("Supermesh Materials names:");
        for (int i = 0; i < receipt.supermeshMaterialsNames.size(); i++)
        {
            PLEEPLOG_DEBUG("    " + receipt.supermeshNames[i] + ":");
            for (auto name : receipt.supermeshMaterialsNames[i])
            {
                PLEEPLOG_DEBUG("        " + name);
            }
        }
        PLEEPLOG_DEBUG("Material names:");
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