#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

//#include "intercession_pch.h"
#include <vector>
#include <unordered_set>
#include <memory>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "rendering/mesh.h"
#include "rendering/material.h"
#include "rendering/armature.h"
#include "rendering/animation_skeletal.h"

namespace pleep
{
    // aiScenes contain Meshes, Materials, Animations and Nodes:
    // Meshes contain: vertices, normals, tangents, bitangents, vertex colors, texture coords, SINGLE material, faces (contains vertex indicies), and bones (contains "map" of vertex id to weight):
    //      populateArmature -> bones contain aiNode* (calulate the bone vector with node Transformation * unit vec?)
    // Materials contain textures
    // Animations contain "channels" aka aiNodeAnim which contains the keyframes indexed to each node
    // Nodes contain a transform and indicies to meshes in the scene. 
    //      nodes are in a tree for transform inheritance

    // Imported assets may not necessarily all neatly correspond to a single entity
    // so using assets will require two steps: importing, which loads named assets into cache
    // and then fetching, which accesses assets based on name keys.
    // so users will have to know both the file, and asset names
    // (alternatively they could import a file and then receive a return for all loaded asset names
    // then they could make some sort of determinition with which to call fetch)

    // A static (global) access point to corrdinate memory for model objects
    //  (as multiple entities could want to use the same data)
    // Methods which allocate gpu memory must be virtual (for ModelManagerFaux)
    class ModelManager
    {
    public:
    
        // List of assets for 1 "collection" from an import
        struct AssetReceipt
        {
            std::unordered_set<std::string> meshNames;
            std::unordered_set<std::string> materialNames; // each material corresponds with mesh at the same index
            std::unordered_set<std::string> armatureNames; // can have more than 1 armature?
            std::unordered_set<std::string> animationNames;
        };

        // Names of assets guaranteed to be cached after a model import.
        //     (whether or not they already existed in cache before this import call)
        // Also used during import to communicate which assets have been loaded during current import
        struct ImportReceipt
        {
            // Name of overall import (file name)
            std::string importName;
            // location import was read from
            std::string importSourceFilepath;

            // index by name of collection's head node name
            //std::unordered_map<std::string, AssetReceipt> collections;

            // list of all unique mesh names from this import
            std::unordered_set<std::string> meshNames;
            // list of all unique material names from this import
            std::unordered_set<std::string> materialNames;
            // list of all unique armature names from this import
            std::unordered_set<std::string> armatureNames;
            // list of all unique animation names from this import
            std::unordered_set<std::string> animationNames;
        };


        // Load all assets from given filepath into cache
        ImportReceipt import(const std::string filepath);

        // Tries to add Material to the cache, constructed with the given dict of texture filepaths
        virtual bool create_material(const std::string& name, const std::unordered_map<TextureType, std::string>& textureDict);

        // ***** Fetch Methods *****
        // if asset exists in library return shallow, const, shared instance from cache
        // (or a deep copy for armatures)
        std::shared_ptr<const Mesh>              fetch_mesh(const std::string& name);
        std::shared_ptr<const Material>          fetch_material(const std::string& name);
        Armature                                 fetch_armature(const std::string& name);
        std::shared_ptr<const AnimationSkeletal> fetch_animation(const std::string& name);
        
        // hardcoded meshes
        enum class BasicMeshType
        {
            // cube with 1m side lengths, normals are not interpolated on corners
            cube,
            // quad in x-y plane with 1m side lengths
            quad,
            // quad in x-y plane with vertices at +/- 1
            screen,
            // regular icosahedron with 1m circumdiameter
            icosahedron,
            // geodesic sphere with 1m diameter
            //sphere,
            // "arrow" pointing along unit vector (0,0,1)
            vector
        };
        // Need a matching string value to use m_meshMap
        // use <> characters because they are illegal for filesnames and wont cause collisions
        // TODO: find an elegant was to convert enum to string
        inline const char* ENUM_TO_STR(BasicMeshType b)
        {
            switch(b)
            {
                case (BasicMeshType::cube):
                    return "<pleep_cube>";
                case (BasicMeshType::quad):
                    return "<pleep_quad>";
                case (BasicMeshType::screen):
                    return "<pleep_screen>";
                case (BasicMeshType::icosahedron):
                    return "<pleep_icosahedron>";
                case (BasicMeshType::vector):
                    return "<pleep_vector>";
                default:
                    return "";
            }
        }
        // convenience method to fetch or generate-and-fetch hardcoded meshes
        // Serialized hardcoded meshes will use the same string-based fetch as other assets.
        std::shared_ptr<const Mesh>  fetch_mesh(const BasicMeshType id);
        
        // Should be called by CosmosContext periodically(?)
        // Remove all models not used anywhere in the Cosmos
        void clear_unused();

        // Clear ALL assets
        // Shared pointers will remain with users whos already fetched them
        void clear_all();
        
        static void debug_scene(const aiScene* scene);
        static void debug_nodes(const aiScene* scene, aiNode* node, const unsigned int depth = 0);
        static void debug_receipt(const ImportReceipt& receipt);

    protected:
        // Maintain assets as shared pointers to live independently of ModelManager object

        // mesh node name -> Mesh 
        // (Distribute only shared_ptr<const Mesh> to not let copies delete GPU memory)
        std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshMap;
        // material name -> Material
        // (Distribute only shared_ptr<const Material> to not let copies delete GPU memory)
        // Modifying materials across ALL entities can only be done through ModelCache methods.
        std::unordered_map<std::string, std::shared_ptr<Material>> m_materialMap;
        // armature node name -> Armature
        // Armatures are individual to each entity and should be copied NOT shared
        std::unordered_map<std::string, Armature> m_armatureMap;
        // armature node name + bone node name -> bone Id
        // bone Id == index in armature.m_bones
        std::unordered_map<std::string, std::unordered_map<std::string, unsigned int>> m_boneIdMapMap;
        // bone name -> armature name
        std::unordered_map<std::string, std::string> m_boneArmatureMap;
        // animation name -> AnimationSkeletal
        // (Distribute only shared_ptr<const Material> to not let copies modify keyframes)
        std::unordered_map<std::string, std::shared_ptr<AnimationSkeletal>> m_animationMap;

        // Check for possible assets (according to format assumptions above) and load into receipt
        // if an asset has no name use nameDefault_assetType_x
        //   where x is the index of that asset in its respective container
        // if omitExisting == true, assets already in library will not be in receipt (to know which need to be loaded)
        ImportReceipt _scan_scene(const aiScene* scene, const std::string& nameDefault = "Pleep", const bool omitExisting = false);
        void _scan_node(const aiScene* scene, const aiNode* node, ImportReceipt& receipt, const bool omitExisting);

        // Iterate through scene materials, load and emplace as new Material in m_materialMap
        // can only search for textures contained exactly in given directory (no postfix / or \\)
        void _process_materials(const aiScene* scene, const ImportReceipt& receipt, const std::string directory = "./");
        virtual std::shared_ptr<Material> _build_material(const aiScene* scene, const aiMaterial* material, const std::string& materialName, const std::string& directory);

        // Iterate through collection nodes for those in scanned receipt armatures,
        //   emplace as Armaturse m_armatureMap named as node name
        void _process_armatures(const aiNode* node, const ImportReceipt& receipt, const glm::mat4 parentTransform = glm::mat4(1.0f));
        virtual Armature _build_armature(const aiNode* node);
        void _extract_bones_from_node(const aiNode* node, std::vector<Bone>& armatureBones, std::string armatureName);

        // Iterate through collection nodes for those in scanned receipt meshes,
        //   emplace as Meshes m_meshMap named as node name
        // use scanned armatures to set vertex bone weights
        // use scanned materials to set receipt meshMaterialsNames
        void _process_meshes(const aiScene* scene, const ImportReceipt& receipt);
        virtual std::shared_ptr<Mesh> _build_mesh(const aiMesh* mesh, const ImportReceipt& receipt);
        void _extract_vertices(std::vector<Vertex>& dest, const aiMesh* src);
        void _extract_indices(std::vector<unsigned int>& dest, const aiMesh* src);
        void _extract_bone_weights_for_vertices(std::vector<Vertex>& dest, const aiMesh* src, const ImportReceipt& receipt);

        // Iterate through scene animations, load and emplace as new AnimationSkeletal in m_animationMap
        void _process_animations(const aiScene* scene, const ImportReceipt& receipt);
        virtual std::shared_ptr<AnimationSkeletal> _build_animation(const aiAnimation* animation);

        // should be congruent with definition of SphereCollider (approximately)
        //virtual std::shared_ptr<Mesh> _build_sphere_mesh();
        // should be congruent with definition of BoxCollider
        virtual std::shared_ptr<Mesh> _build_cube_mesh();
        virtual std::shared_ptr<Mesh> _build_quad_mesh();
        virtual std::shared_ptr<Mesh> _build_screen_mesh();
        virtual std::shared_ptr<Mesh> _build_icosahedron_mesh();
        virtual std::shared_ptr<Mesh> _build_vector_mesh();
    };
}

#endif // MODEL_MANAGER_H