#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

//#include "intercession_pch.h"
#include <vector>
#include <memory>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "rendering/supermesh.h"
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
        // Names of assets guaranteed to be cached after a model import.
        //     (whether or not they already existed in cache before this import call)
        // Also used during import to communicate which assets have been loaded during current import
        struct ImportReceipt
        {
            // Name of overall import (also used as deafult base name for unnamed assets)
            std::string importName;
            // location import was read from
            std::string importSourceFilepath;

            // list of all unique Supermesh names from this import
            std::vector<std::string> supermeshNames;
            // each supermeshSubmeshesNames entry corresponds with the supermeshName at the same index
            // each supermeshSubmeshesNames ENTRY'S entry corresponds with the submesh at the same index
            std::vector<std::vector<std::string>> supermeshSubmeshesNames;
            // each supermeshMaterialsNames entry corresponds with the supermeshName at the same index
            // each supermeshMaterialsNames ENTRY'S entry corresponds with the submesh at the same index
            std::vector<std::vector<std::string>> supermeshMaterialsNames;

            // list of all unique material names from this import
            std::vector<std::string> materialNames;
            // list of all unique armature names from this import
            std::vector<std::string> armatureNames;
            // list of all unique animation names from this import
            std::vector<std::string> animationNames;
        };

        // Load all assets from given filepath into cache
        ImportReceipt import(const std::string filepath);

        // Tries to add Material to the cache, constructed with the given textureDict
        virtual bool create_material(const std::string& name, const std::unordered_map<TextureType, std::string>& textureDict);

        // ***** Fetch Methods *****
        // if asset exists in library return shallow, const, shared instance from cache
        // (or a deep copy for armatures)
        std::shared_ptr<const Supermesh>         fetch_supermesh(const std::string& name);
        std::shared_ptr<const Material>          fetch_material(const std::string& name);
        std::shared_ptr<Armature>                fetch_armature(const std::string& name);
        std::shared_ptr<const AnimationSkeletal> fetch_animation(const std::string& name);
        
        // hardcoded supermeshes
        enum class BasicSupermeshType
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
        };
        // Need a matching string value to use m_supermeshMap
        // use <> characters because they are illegal for filesnames and wont cause collisions
        // TODO: find an elegant was to convert enum to string
        inline const char* ENUM_TO_STR(BasicSupermeshType b)
        {
            switch(b)
            {
                case (BasicSupermeshType::cube):
                    return "<pleep_cube>";
                case (BasicSupermeshType::quad):
                    return "<pleep_quad>";
                case (BasicSupermeshType::screen):
                    return "<pleep_screen>";
                case (BasicSupermeshType::icosahedron):
                    return "<pleep_icosahedron>";
                default:
                    return "";
            }
        }
        // convenience method to fetch or generate-and-fetch hardcoded supermeshes
        // Serialized hardcoded supermeshes will use the same string-based fetch as other assets.
        std::shared_ptr<const Supermesh>  fetch_supermesh(const BasicSupermeshType id);
        
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

        // mesh node name -> Supermesh 
        // (Distribute only shared_ptr<const Superesh> to not let copies delete GPU memory)
        std::unordered_map<std::string, std::shared_ptr<Supermesh>> m_supermeshMap;
        // material name -> Material
        // (Distribute only shared_ptr<const Material> to not let copies delete GPU memory)
        // Modifying materials across ALL entities can only be done through ModelCache methods.
        std::unordered_map<std::string, std::shared_ptr<Material>> m_materialMap;
        // armature node name -> Armature
        // Armatures are individual to each entity and should be copied NOT shared
        std::unordered_map<std::string, std::shared_ptr<Armature>> m_armatureMap;
        // animation name -> AnimationSkeletal
        // (Distribute only shared_ptr<const AnimationSkeletal> to not let copies modify)
        // Animation data is shared between entities, animation state is in individual animation component
        std::unordered_map<std::string, std::shared_ptr<AnimationSkeletal>> m_animationMap;

        // Check for possible assets (according to format assumptions above) and load into receipt
        // if an asset has no name use nameDefault_assetType_x
        //   where x is the index of that asset in its respective container
        // Use omitDuplicates = true for scanning assets which need to be created
        // Use omitDuplicates = false for scanning assets which need to be reported
        ImportReceipt _scan_scene(const aiScene* scene, const std::string& nameDefault = "Pleep", const bool omitDuplicates = true);

        // Iterate through scene materials, load and emplace as new Material in m_materialMap
        // can only search for textures contained exactly in given directory (no postfix / or \\)
        void _process_materials(const aiScene* scene, const ImportReceipt& receipt, const std::string directory = "./");
        virtual std::shared_ptr<Material> _build_material(const aiMaterial* material, const std::string& materialName, const std::string& directory);

        // Iterate through root nodes for those in scanned receipt armatures,
        //   emplace as Armaturse m_armatureMap named as node name
        void _process_armatures(const aiScene* scene, const ImportReceipt& receipt);
        virtual std::shared_ptr<Armature> _build_armature(const aiNode* node, const std::string& armatureName);
        void _extract_bones_from_node(const aiNode* node, std::vector<Bone>& armatureBones, std::unordered_map<std::string, unsigned int>& armatureBoneIdMap);

        // Iterate through root nodes for those in scanned receipt meshes,
        //   emplace as Supermeshes m_supermeshMap named as node name
        // use scanned armatures to set vertex bone weights
        // use scanned materials to set receipt supermeshMaterialsNames
        void _process_supermeshes(const aiScene* scene, const ImportReceipt& receipt);
        std::shared_ptr<Supermesh> _build_supermesh(const aiScene* scene, const aiNode* node, const ImportReceipt& receipt);
        virtual std::shared_ptr<Mesh> _build_mesh(const aiMesh* mesh, const ImportReceipt& receipt);
        void _extract_vertices(std::vector<Vertex>& dest, const aiMesh* src);
        void _extract_indices(std::vector<unsigned int>& dest, const aiMesh* src);
        void _extract_bone_weights_for_vertices(std::vector<Vertex>& dest, const aiMesh* src, const ImportReceipt& receipt);

        // Iterate through scene animations, load and emplace as new AnimationSkeletal in m_animationMap
        void _process_animations(const aiScene* scene, const ImportReceipt& receipt);
        virtual std::shared_ptr<AnimationSkeletal> _build_animation(const aiAnimation* animation, const std::string& animationName);

        virtual std::shared_ptr<Supermesh> _build_cube_supermesh();
        virtual std::shared_ptr<Supermesh> _build_quad_supermesh();
        virtual std::shared_ptr<Supermesh> _build_screen_supermesh();
        virtual std::shared_ptr<Supermesh> _build_icosahedron_supermesh();
    };
}

#endif // MODEL_MANAGER_H