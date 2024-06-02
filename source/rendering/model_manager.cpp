#include "rendering/model_manager.h"

#include <cassert>

#include "logging/pleep_log.h"
#include "rendering/assimp_converters.h"

namespace pleep
{
    ModelManager::ImportReceipt ModelManager::import(const std::string filepath)
    {
        // hardcoded assets need to be able to use the same import pathway as 
        // other meshes for ambiguous deserialization
        if (filepath == ModelManager::ENUM_TO_STR(BasicMeshType::cube))
        {
            this->m_meshMap[filepath] = this->_build_cube_mesh();
            this->m_meshMap[filepath]->m_name = filepath;
            this->m_meshMap[filepath]->m_sourceFilepath = filepath;
            return ImportReceipt{filepath, filepath, {{filepath,{{filepath}}}}, {filepath}};
        }
        else if (filepath == ModelManager::ENUM_TO_STR(BasicMeshType::quad))
        {
            this->m_meshMap[filepath] = this->_build_quad_mesh();
            this->m_meshMap[filepath]->m_name = filepath;
            this->m_meshMap[filepath]->m_sourceFilepath = filepath;
            return ImportReceipt{filepath, filepath, {{filepath,{{filepath}}}}, {filepath}};
        }
        else if (filepath == ModelManager::ENUM_TO_STR(BasicMeshType::screen))
        {
            this->m_meshMap[filepath] = this->_build_screen_mesh();
            this->m_meshMap[filepath]->m_name = filepath;
            this->m_meshMap[filepath]->m_sourceFilepath = filepath;
            return ImportReceipt{filepath, filepath, {{filepath,{{filepath}}}}, {filepath}};
        }
        else if (filepath == ModelManager::ENUM_TO_STR(BasicMeshType::icosahedron))
        {
            this->m_meshMap[filepath] = this->_build_icosahedron_mesh();
            this->m_meshMap[filepath]->m_name = filepath;
            this->m_meshMap[filepath]->m_sourceFilepath = filepath;
            return ImportReceipt{filepath, filepath, {{filepath,{{filepath}}}}, {filepath}};
        }
        else if (filepath == ModelManager::ENUM_TO_STR(BasicMeshType::vector))
        {
            this->m_meshMap[filepath] = this->_build_vector_mesh();
            this->m_meshMap[filepath]->m_name = filepath;
            this->m_meshMap[filepath]->m_sourceFilepath = filepath;
            return ImportReceipt{filepath, filepath, {{filepath,{{filepath}}}}, {filepath}};
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
        const aiScene *scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PopulateArmatureData);
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
        ImportReceipt scan = this->_scan_scene(scene, filestem, true);
        scan.importSourceFilepath = filepath;
        debug_receipt(scan);
        
        // process materials and armatures first for meshes to reference
        this->_process_materials(scene, scan, directory);
        this->_process_armatures(scene->mRootNode, scan);//, glm::inverse(assimp_converters::convert_matrix4(scene->mRootNode->mTransformation)));
        this->_process_meshes(scene, scan);
        this->_process_animations(scene, scan);

        // now all assets are cached, user can call fetch methods using receipt names
        // return ALL assets available in scene
        ImportReceipt receipt = this->_scan_scene(scene, filestem);
        receipt.importSourceFilepath = filepath;
        //debug_receipt(receipt);
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

    std::shared_ptr<const Mesh> ModelManager::fetch_mesh(const std::string& name)
    {
        auto meshIt = this->m_meshMap.find(name);
        if (meshIt == this->m_meshMap.end())
        {
            PLEEPLOG_WARN("Mesh " + name + " is not cached.");
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

    Armature ModelManager::fetch_armature(const std::string& name)
    {
        auto armatureIt = this->m_armatureMap.find(name);
        if (armatureIt == this->m_armatureMap.end())
        {
            PLEEPLOG_WARN("Armature " + name + " is not cached.");
            return Armature{};
        }
        else
        {
            // return copy (copy constructor must be a deep copy)
            return armatureIt->second;
        }
    }

    std::shared_ptr<const AnimationSkeletal> ModelManager::fetch_animation(const std::string& name)
    {
        auto animationIt = this->m_animationMap.find(name);
        if (animationIt == this->m_animationMap.end())
        {
            PLEEPLOG_WARN("Animation " + name + " is not cached.");
            return nullptr;
        }
        else
        {
            return animationIt->second;
        }
    }
    
    std::shared_ptr<const Mesh> ModelManager::fetch_mesh(const ModelManager::BasicMeshType id)
    {
        std::string polyName = ModelManager::ENUM_TO_STR(id);
        if (polyName == "") return nullptr;
        
        // check if basic mesh is in cache from previous call
        auto meshIt = this->m_meshMap.find(polyName);
        if (meshIt != this->m_meshMap.end())
        {
            return meshIt->second;
        }

        // otherwise, since "filepath" and name are identical we can import it
        this->import(polyName);
        // and then return it in this single call
        return this->m_meshMap[polyName];
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
        this->m_meshMap.clear();
        this->m_materialMap.clear();
        this->m_armatureMap.clear();
        this->m_animationMap.clear();
    }
    
    ModelManager::ImportReceipt ModelManager::_scan_scene(const aiScene* scene, const std::string& nameDefault, const bool omitExisting) 
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
            if (omitExisting && (m_materialMap.count(matName) || receipt.materialNames.count(matName)))
            {
                PLEEPLOG_WARN("Could not import material " + matName + ", it already exists");
                // we could try to generate a default name here if file name is different
                continue;
            }

            receipt.materialNames.insert(matName);
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
            if (omitExisting && (m_animationMap.count(animName) || receipt.animationNames.count(animName)))
            {
                PLEEPLOG_WARN("Could not import animation " + animName + ", it already exists");
                // we could try to generate a default name here? unless this already is the deafult name
                continue;
            }

            receipt.animationNames.insert(animName);
        }

        // all meshes should be directly in scene
        for (unsigned int i = 0; i < scene->mNumMeshes; i++)
        {
            std::string meshName = scene->mMeshes[i]->mName.C_Str();
            if (meshName.empty())
            {
                meshName = receipt.importName + "_mesh_" + std::to_string(i);
            }
            
            // check uniqueness
            if (omitExisting && (m_meshMap.count(meshName) || receipt.meshNames.count(meshName)))
            {
                PLEEPLOG_WARN("Could not import mesh " + meshName + ", it already exists");
                // we could try to generate a default name here? unless this already is the deafult name
                continue;
            }

            receipt.meshNames.insert(meshName);
        }

        // can scene not have root node?
        // scene.h::255 > "There will always be at least the root node if the import was successful"
        assert(scene->mRootNode);

        // in GLB export if there are multiple objects, then root node will be called ROOT
        // so if root node has name ROOT then treat each of its children as a collection
        // otherwise root node is the only collection
        // Is there any better way to recognize this?

        // for now we'll just forget collections and import everything flat
        // come back later if there is a need for better organization

        _scan_node(scene, scene->mRootNode, receipt, omitExisting);

        return receipt;
    }
    
    void ModelManager::_scan_node(const aiScene* scene, const aiNode* node, ImportReceipt& receipt, const bool omitExisting)
    {
        // check if I have meshes
        for (unsigned int m = 0; m < node->mNumMeshes; m++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[m]];

            // do those meshes have any bones and therefore armatures?
            if (mesh->HasBones())
            {
                // only need to check first bone?
                const std::string armatureName = mesh->mBones[0]->mArmature->mName.C_Str();
                if (omitExisting && (m_armatureMap.count(armatureName) || receipt.armatureNames.count(armatureName)))
                {
                    PLEEPLOG_WARN("Could not import armature " + armatureName + ", it already exists");
                    // we could try to generate a default name here? unless this already is the default name
                    continue;
                }
                receipt.armatureNames.insert(armatureName);
            }
        }

        for (unsigned int rootIndex = 0; rootIndex < node->mNumChildren; rootIndex++)
        {
            aiNode* child = node->mChildren[rootIndex];
            _scan_node(scene, child, receipt, omitExisting);
        }

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
            if (receipt.materialNames.count(materialName) == 0) continue;

            m_materialMap[materialName] = _build_material(scene, material, materialName, directory);
            m_materialMap[materialName]->m_name = materialName;
            m_materialMap[materialName]->m_sourceFilepath = receipt.importSourceFilepath;
        }
    }

    std::shared_ptr<Material> ModelManager::_build_material(const aiScene* scene, const aiMaterial *material, const std::string& materialName, const std::string& directory)
    {
        PLEEPLOG_DEBUG("Loading material: " + materialName);
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
                
                const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());

                // nullptr means it was not embedded
                if (embeddedTexture == nullptr)
                {
                    std::string filepath = str.C_Str();
                    if (!directory.empty())
                        filepath = directory + '/' + filepath;
                    PLEEPLOG_DEBUG("Loading texture: " + filepath);

                    // weirdness to pass Texture constructor parameters
                    loadedTextures.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(static_cast<TextureType>(type)),
                        std::forward_as_tuple(static_cast<TextureType>(type), filepath)
                    );
                }
                else
                {
                    std::string filepath = embeddedTexture->mFilename.C_Str();
                    PLEEPLOG_DEBUG("Texture: " + filepath + " was embedded, LOADING THIS IS NOT IMPLEMENTED, IGNORING!");
                }
            }
        }

        return std::make_shared<Material>(std::move(loadedTextures));
    }

    void ModelManager::_process_armatures(const aiNode *node, const ImportReceipt& receipt, const glm::mat4 parentTransform)
    {
        // crawl through nodes, if we find a node name which matches armature required by receipt
        // then create an armature from it and all descendants.
        const std::string nodeName = node->mName.C_Str();
        if (receipt.armatureNames.count(nodeName))
        {
            // name should be guarenteed because it is referenced by bone?
            m_armatureMap[nodeName] = _build_armature(node);
            m_armatureMap[nodeName].m_name = nodeName;
            m_armatureMap[nodeName].m_sourceFilepath = receipt.importSourceFilepath;
            // apply parent transform at this point
            m_armatureMap[nodeName].m_relativeTransform = parentTransform;
        }
        else
        {
            const glm::mat4 nodeTransform = parentTransform * assimp_converters::convert_matrix4(node->mTransformation);
            // keep looking
            for (unsigned int i = 0; i < node->mNumChildren; i++)
            {
                _process_armatures(node->mChildren[i], receipt, nodeTransform);
            }
        }
    }

    Armature ModelManager::_build_armature(const aiNode* node)
    {
        const std::string nodeName = node->mName.C_Str();
        PLEEPLOG_DEBUG("Loading armature: " + nodeName);
        std::vector<Bone> armatureBones;

        // assume every descendant node is a bone? including ourself
        _extract_bones_from_node(node, armatureBones, nodeName);

        return Armature{armatureBones};
    }

    
    void ModelManager::_extract_bones_from_node(const aiNode* node, std::vector<Bone>& armatureBones, std::string armatureName)
    {
        // add this bone to map
        const unsigned int thisBoneId = static_cast<unsigned int>(armatureBones.size());
        const std::string thisBoneName = node->mName.C_Str();
        // node name should be guarenteed to be valid for bones

        // add this node to armature
        armatureBones.push_back(
            Bone(thisBoneName, thisBoneId, assimp_converters::convert_matrix4(node->mTransformation))
        );
        // Bone will be missing inverse bind matrix, which is set by the mesh nodes

        // link bone to armature
        m_boneIdMapMap[armatureName][thisBoneName] = thisBoneId;
        m_boneArmatureMap[thisBoneName] = armatureName;

        // recurse
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            // we know what id our child will pick before we recurse so we can save them easily now
            armatureBones[thisBoneId].m_childIds.push_back(static_cast<unsigned int>(armatureBones.size()));
            _extract_bones_from_node(node->mChildren[i], armatureBones, armatureName);
        }
    }
    
    void ModelManager::_process_meshes(const aiScene* scene, const ImportReceipt& receipt) 
    {
        for (unsigned int m = 0; m < scene->mNumMeshes; m++)
        {
            aiMesh* mesh = scene->mMeshes[m];
            std::string meshName = mesh->mName.C_Str();
            // use same rename rules
            if (meshName.empty())
            {
                meshName = receipt.importName + "_mesh_" + std::to_string(m);
            }

            // check for meshes only in scan
            if (receipt.meshNames.count(meshName) <= 0 || m_meshMap.count(meshName) > 0)
            {
                continue;
            }

            m_meshMap[meshName] = _build_mesh(mesh, receipt);
            // apply name outside of _build_mesh for ModelManagerFaux
            m_meshMap[meshName]->m_name = meshName;
            m_meshMap[meshName]->m_sourceFilepath = receipt.importSourceFilepath;
        }
    }

    std::shared_ptr<Mesh> ModelManager::_build_mesh(const aiMesh *mesh, const ImportReceipt& receipt)
    {
        PLEEPLOG_DEBUG("Loading mesh " + std::string(mesh->mName.C_Str()));
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // extract vertices
        _extract_vertices(vertices, mesh);
        //PLEEPLOG_DEBUG("Loaded vertices");
        // extract indices
        _extract_indices(indices, mesh);
        //PLEEPLOG_DEBUG("Loaded indices");

        // extract bone weights
        // setup bone data for each vertex iterating by bones, not by vertices like above
        _extract_bone_weights_for_vertices(vertices, mesh, receipt);
        //PLEEPLOG_DEBUG("Loaded bone weights");

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
        //PLEEPLOG_DEBUG("Mesh has bones: " + std::to_string(src->mNumBones));
        // parse through assimp bone list
        for (unsigned int boneIndex = 0; boneIndex < src->mNumBones; boneIndex++)
        {
            aiBone* boneData = src->mBones[boneIndex];
            // use bone name or bone->mNode name? We use node name elsewhere
            std::string boneName = boneData->mNode->mName.C_Str();
            // PLEEPLOG_DEBUG("Found bone: " + boneName);

            // find bone armature
            std::string armatureName = boneData->mArmature->mName.C_Str();
            //PLEEPLOG_DEBUG("Found armature: " + armatureName);
            // TODO: empty armature name?

            // check if armature exists (created earlier this import)
            if (m_armatureMap.count(armatureName) == 0)
            {
                PLEEPLOG_WARN("Armature " + armatureName + " for bone doesn't exist?");
                continue;
            }

            // check if armature is part of THIS import
            if (receipt.armatureNames.count(armatureName) == 0)
            {
                PLEEPLOG_WARN("Armature " + armatureName + " isn't part of this import?");
                continue;
            }

            // check if bone exists in imported armature
            auto boneIdIt = m_boneIdMapMap[armatureName].find(boneName);
            if (boneIdIt == m_boneIdMapMap[armatureName].end())
            {
                PLEEPLOG_WARN("Bone " + boneName + " does not exist in armature " + armatureName);
                continue;
            }
            // get bone id
            unsigned int boneId = boneIdIt->second;
            // PLEEPLOG_DEBUG("Which is Id: " + std::to_string(boneId));

            // while we're here, set mesh to bone transform in armature
            // PLEEPLOG_DEBUG("Setting bone transform in armature: " + armatureName);
            m_armatureMap[armatureName].m_bones[boneId].m_bindTransform = assimp_converters::convert_matrix4(boneData->mOffsetMatrix);

            // now to actually set the weights...
            // fetch corresponding weight per vertex for *this* bone
            aiVertexWeight* vertexWeights = boneData->mWeights;

            // PLEEPLOG_DEBUG("Setting weights: " + std::to_string(boneData->mNumWeights));
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
            std::string animationName = animation->mName.C_Str();
            if (animationName.empty())
            {
                animationName = receipt.importName + "_animation_" + std::to_string(i);
            }
            PLEEPLOG_DEBUG("Loading anime-tion " + animationName);

            m_animationMap[animationName] = _build_animation(animation);

            // TODO: provide name, filepath, and ALL data required for serialization OUTSIDE of _build_animation, so that ModelManagerFaux's life is easy
            m_animationMap[animationName]->m_name = animationName;
            m_animationMap[animationName]->m_sourceFilepath = receipt.importSourceFilepath;
        }
    }

    std::shared_ptr<AnimationSkeletal> ModelManager::_build_animation(const aiAnimation *animation)
    {
        std::shared_ptr<AnimationSkeletal> newAnimation = std::make_shared<AnimationSkeletal>();
        newAnimation->m_duration = animation->mDuration;
        newAnimation->m_frequency = animation->mTicksPerSecond;

        // For each boneId, extract each keyframe
        for (unsigned int channelId = 0; channelId < animation->mNumChannels; channelId++)
        {
            const aiNodeAnim* nodeAnim = animation->mChannels[channelId];
            //PLEEPLOG_DEBUG("Fetching channel " + std::to_string(channelId));
            const std::string boneName = nodeAnim->mNodeName.C_Str();
            //PLEEPLOG_DEBUG("Fetching bone " + boneName);
            const std::string armatureName = m_boneArmatureMap[boneName];
            //PLEEPLOG_DEBUG("Fetching armature " + armatureName);

            // channels indices do not align with bone indices
            // so we need to lookup its real boneId
            const unsigned int boneId = m_boneIdMapMap[armatureName][boneName];

            //PLEEPLOG_DEBUG("Loading " + std::to_string(nodeAnim->mNumPositionKeys) + " position keyframes for: " + boneName);
            for (unsigned int t = 0; t < nodeAnim->mNumPositionKeys; t++)
            {
                newAnimation->m_posKeyframes[boneId].push_back({
                    assimp_converters::convert_vec3(nodeAnim->mPositionKeys[t].mValue),
                    nodeAnim->mPositionKeys[t].mTime
                });
            }
            //PLEEPLOG_DEBUG("Loading " + std::to_string(nodeAnim->mNumRotationKeys) + " rotation keyframes for: " + boneName);
            for (unsigned int t = 0; t < nodeAnim->mNumRotationKeys; t++)
            {
                newAnimation->m_rotKeyframes[boneId].push_back({
                    assimp_converters::convert_quat(nodeAnim->mRotationKeys[t].mValue),
                    nodeAnim->mRotationKeys[t].mTime
                });
            }
            //PLEEPLOG_DEBUG("Loading " + std::to_string(nodeAnim->mNumScalingKeys) + " scaling keyframes for: " + boneName);
            for (unsigned int t = 0; t < nodeAnim->mNumScalingKeys; t++)
            {
                newAnimation->m_sclKeyframes[boneId].push_back({
                    assimp_converters::convert_vec3(nodeAnim->mScalingKeys[t].mValue),
                    nodeAnim->mScalingKeys[t].mTime
                });
            }
        }
        return newAnimation;
    }

    std::shared_ptr<Mesh> ModelManager::_build_cube_mesh() 
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

        return std::make_shared<Mesh>(vertices, indices);
    }
    
    std::shared_ptr<Mesh> ModelManager::_build_quad_mesh() 
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

        return std::make_shared<Mesh>(vertices, indices);
    }
    
    std::shared_ptr<Mesh> ModelManager::_build_screen_mesh() 
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

        return std::make_shared<Mesh>(vertices, indices);
    }
    
    std::shared_ptr<Mesh> ModelManager::_build_icosahedron_mesh() 
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

        return std::make_shared<Mesh>(vertices, indices);
    }
    
    std::shared_ptr<Mesh> ModelManager::_build_vector_mesh()
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

        return std::make_shared<Mesh>(vertices, indices);
    }

    void ModelManager::debug_scene(const aiScene *scene)
    {
        PLEEPLOG_DEBUG("Scene name: " + std::string(scene->mName.C_Str()));
        PLEEPLOG_DEBUG("    meshes: " + std::to_string(scene->mNumMeshes));
        PLEEPLOG_DEBUG("    materials: " + std::to_string(scene->mNumMaterials));
        PLEEPLOG_DEBUG("    textures: " + std::to_string(scene->mNumTextures));
        PLEEPLOG_DEBUG("    animations: " + std::to_string(scene->mNumAnimations));
        
        PLEEPLOG_DEBUG("Root node:");
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
/* 
        PLEEPLOG_DEBUG("Collection names:");
        for (auto pair : receipt.collections)
        {
            PLEEPLOG_DEBUG("    " + pair.first);
            PLEEPLOG_DEBUG("     Mesh names:");
            for (auto name : pair.second.meshNames)
            {
                PLEEPLOG_DEBUG("        " + name);
            }
            PLEEPLOG_DEBUG("     Material names:");
            for (auto name : pair.second.materialNames)
            {
                PLEEPLOG_DEBUG("        " + name);
            }
            PLEEPLOG_DEBUG("     Armature names:");
            for (auto name : pair.second.armatureNames)
            {
                PLEEPLOG_DEBUG("        " + name);
            }
            PLEEPLOG_DEBUG("     Animation names:");
            for (auto name : pair.second.animationNames)
            {
                PLEEPLOG_DEBUG("        " + name);
            }
        }
*/
        PLEEPLOG_DEBUG("All Mesh names:");
        for (auto name : receipt.meshNames)
        {
            PLEEPLOG_DEBUG("    " + name);
        }
        PLEEPLOG_DEBUG("All Material names:");
        for (auto name : receipt.materialNames)
        {
            PLEEPLOG_DEBUG("    " + name);
        }
        PLEEPLOG_DEBUG("All Armature names:");
        for (auto name : receipt.armatureNames)
        {
            PLEEPLOG_DEBUG("    " + name);
        }
        PLEEPLOG_DEBUG("All Animation names:");
        for (auto name : receipt.animationNames)
        {
            PLEEPLOG_DEBUG("    " + name);
        }
        PLEEPLOG_DEBUG("Done receipt");
    }
}