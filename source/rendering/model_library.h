#ifndef MODEL_LIBRARY_H
#define MODEL_LIBRARY_H

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
    const std::string MODEL_DIRECTORY_PATH = "resources/";

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
    class ModelLibrary
    {
    protected:
        // Cannot log from this constructor because it is invoked before logger init in main
        ModelLibrary() = default;
    public:
        ~ModelLibrary() = default;
        // no copy constructor
        ModelLibrary(const ModelLibrary&) = delete;
        

        // Names of assets guaranteed to be cached after a model import.
        // Also used during import communicate down the pipeline which assets were loaded in current import 
        struct ImportReceipt
        {
            // Name of overall import (also used as deafult base name for unnamed assets)
            std::string importName;
            // location import was read from
            std::string importSourceFilepath;

            // list of all unique Supermesh names from this import
            std::vector<std::string> supermeshNames;
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
        /*
            Assumptions:
        a. We won't record the model filename, only asset names, so users need to know the file & asset (or use the receipt names)
        b. Importing an asset twice will load it twice, but it will NOT override the assets already in cache
        c. Importing another asset with a name collision, will not override the first!
        d. Models and armatures are only DIRECT children of the root node. Models are nodes with at least 1 mesh, and armatures are any other node.
        e. Different meshes with bones that reference the same armature node are either the same bone (bones are unique) OR they have the same bone-space to mesh-space transform (bones are identical). If not, the last mesh will take priority
    
            Process:
        1. Scan the scene for possible assets (see assumptions b, c and d)
        2. Cache a Material for every material in the scene scan
        3. Cache an Armature for every node in the root node AND in the scene scan armatures
            (give each node a unique packed id, and create a nodeIdMap from their name string)
        3. Cache a Supermesh for every node in the root node and in the scene scan meshes
            (+ associate each submesh with its material name in the receipt)
            (+ if a submesh has bones associate each vertex with the bones from its armature)
        4. Cache an AnimationSkeletal for every animation in the scene
        5. Cache an animation for every animation in the scene
            (NodeAnim names reference node/bone names, so we'll need to map use armature nodeIdMaps)
        
        TODO: make this async and set futures in receipt?
        */
        static ModelLibrary::ImportReceipt import(std::string filepath);

        // returns false if name was taken
        // (Should instead generate an available name and return it as a receipt?)
        //static bool create_material(const std::string& name, std::unordered_map<TextureType, std::string>& textureDict);

        // ***** Fetch Methods *****
        // if asset exists in library return shallow, const, shared instance from cache
        // (or a deep copy for armatures)
        static std::shared_ptr<const Supermesh>         fetch_supermesh(const std::string& name);
        static std::shared_ptr<const Material>          fetch_material(const std::string& name);
        static std::shared_ptr<Armature>                fetch_armature(const std::string& name);
        static std::shared_ptr<const AnimationSkeletal> fetch_animation(const std::string& name);
        
        // fetch or generate-and-fetch hardcoded supermeshes:
        // cube with 1m side lengths, normals are not interpolated on corners
        static std::shared_ptr<const Supermesh>         fetch_cube_supermesh();
        // quad in x-y plane with 1m side lengths
        static std::shared_ptr<const Supermesh>         fetch_quad_supermesh();
        // quad in x-y plane with
        static std::shared_ptr<const Supermesh>         fetch_screen_supermesh();

        // Should be called by CosmosContext periodically(?)
        // Remove all models not used anywhere in the Cosmos
        static void clear_unused();

        // Clear ALL assets
        // Shared pointers will remain with users whos already fetched them
        static void clear_library();

        // ***** Misc Utils *****
        // uses flag id of -1 to disable bone
        static void set_vertex_bone_data_to_default(Vertex& vertex);
        // finds next disabled bone, and sets id and weight. Sets nothing if all 4 are valid bones
        static void set_vertex_bone_data(Vertex& vertex, int boneId, float weight);

    protected:
        // users don't need to be able to literally 'get' this instance, so it can be unique
        // initialized in model_library.cpp
        static std::unique_ptr<ModelLibrary> m_singleton;


        // Maintain assets as shared pointers to live independently after a clear_library()

        // mesh node name -> Supermesh 
        // (Distribute only shared_ptr<const Superesh> to not let copies delete GPU memory)
        std::unordered_map<std::string, std::shared_ptr<Supermesh>> m_supermeshMap;
        // material name -> Material
        // (Distribute only shared_ptr<const Material> to not let copies delete GPU memory)
        // Modifying materials across ALL entities can only be done through ModelLibrary methods.
        std::unordered_map<std::string, std::shared_ptr<Material>> m_materialMap;
        // armature node name -> Armature
        // Armatures are individual to each entity and should be copied NOT shared
        std::unordered_map<std::string, std::shared_ptr<Armature>> m_armatureMap;
        // animation name -> AnimationSkeletal
        // (Distribute only shared_ptr<const AnimationSkeletal> to not let copies modify)
        // Animation data is shared between entities, animation state is in individual animation component
        std::unordered_map<std::string, std::shared_ptr<AnimationSkeletal>> m_animationMap;

        // special map keys for hardcoded supermeshes
        // use '<>' characters because they are illegal filename characters, so there will be no collisions
        const std::string cubeKey =     "<pleep_cube>";
        const std::string quadKey =     "<pleep_quad>";
        const std::string screenKey =   "<pleep_screen>";

        // Check for possible assets (according to format assumptions above) and load into receipt
        // if an asset has no name use nameDeafult_assettype_X
        //   where X is the index of that asset in its respective container
        ModelLibrary::ImportReceipt _scan_scene(const aiScene* scene, const std::string& nameDefault = "Pleep");

        // Iterate through scene materials, load and emplace as new Material in m_materialMap
        // can only search for textures contained exactly in given directory (no postfix / or \\)
        void _process_materials(const aiScene* scene, ImportReceipt& receipt, const std::string directory = "./");
        std::shared_ptr<Material> _build_material(aiMaterial* material, const std::string& materialName, const std::string& directory);

        // Iterate through root nodes for those in scanned receipt armatures,
        //   emplace as Armaturse m_armatureMap named as node name
        void _process_armatures(const aiScene* scene, ImportReceipt& receipt);
        std::shared_ptr<Armature> _build_armature(aiNode* node, const std::string& armatureName);
        void _extract_bones_from_node(aiNode* node, std::vector<Bone>& armatureBones, std::unordered_map<std::string, unsigned int>& armatureBoneIdMap);

        // Iterate through root nodes for those in scanned receipt meshes,
        //   emplace as Supermeshes m_supermeshMap named as node name
        // use scanned armatures to set vertex bone weights
        // use scanned materials to set receipt supermeshMaterialsNames
        void _process_supermeshes(const aiScene* scene, ImportReceipt& receipt);
        std::shared_ptr<Mesh> _build_mesh(aiMesh* mesh, ImportReceipt& receipt);
        void _extract_vertices(std::vector<Vertex>& dest, aiMesh* src);
        void _extract_indices(std::vector<unsigned int>& dest, aiMesh* src);
        void _extract_bone_weights_for_vertices(std::vector<Vertex>& dest, aiMesh* src, ImportReceipt& receipt);

        // Iterate through scene animations, load and emplace as new AnimationSkeletal in m_animationMap
        void _process_animations(const aiScene* scene, ImportReceipt& receipt);
        std::shared_ptr<AnimationSkeletal> _build_animation(aiAnimation* animation, const std::string& animationName);

        std::shared_ptr<Supermesh> _build_cube_supermesh();
        std::shared_ptr<Supermesh> _build_quad_supermesh();
        std::shared_ptr<Supermesh> _build_screen_supermesh();

        static void _debug_scene(const aiScene* scene);
        static void _debug_nodes(const aiScene* scene, aiNode* node, const unsigned int depth = 0);
        static void _debug_receipt(const ImportReceipt& receipt);
    };
}

#endif // MODEL_LIBRARY_H