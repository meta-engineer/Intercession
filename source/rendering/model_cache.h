#ifndef MODEL_CACHE_H
#define MODEL_CACHE_H

//#include "intercession_pch.h"
#include <memory>

#include "rendering/model_manager.h"

#include "rendering/supermesh.h"
#include "rendering/material.h"
#include "rendering/armature.h"
#include "rendering/animation_skeletal.h"

namespace pleep
{
    // Facade for access to global ModelManager instance
    namespace ModelCache
    {
        // Shared ModelManager instance
        // Cannot log from constructor because it is invoked before logger init in main
        // TODO: refactor to be a static member of a ModelCache class?
        extern std::unique_ptr<ModelManager> g_modelManager;

        // "facaditory" duplicate ModelManager's ImportReceipt type
        using ImportReceipt = ModelManager::ImportReceipt;
        // "facaditory" duplicate ModelManager's BasicSupermeshType type
        using BasicSupermeshType = ModelManager::BasicSupermeshType;

        // Load all assets from given filepath into cache
        inline ImportReceipt import(std::string filepath)
        { return g_modelManager->import(filepath); }

        // Tries to add Material to the cache, constructed with the given textureDict
        // returns true if it was successful (implied ImportReceipt)
        // returns false if name was taken
        // (Should instead generate an available name and return it as a receipt?)
        inline bool create_material(const std::string& name, const std::unordered_map<TextureType, std::string>& textureDict)
        { return g_modelManager->create_material(name, textureDict); }

        // ***** Fetch Methods *****
        // If asset exists in cache return shallow, const, shared instance from cache
        // (or a deep copy for armatures)
        inline std::shared_ptr<const Supermesh>         fetch_supermesh(const std::string& name)
        { return g_modelManager->fetch_supermesh(name); }
        inline std::shared_ptr<const Material>          fetch_material(const std::string& name)
        { return g_modelManager->fetch_material(name); }
        inline std::shared_ptr<Armature>                fetch_armature(const std::string& name)
        { return g_modelManager->fetch_armature(name); }
        inline std::shared_ptr<const AnimationSkeletal> fetch_animation(const std::string& name)
        { return g_modelManager->fetch_animation(name); }

        // convenience method to fetch or generate-and-fetch hardcoded supermeshes
        // Serialized hardcoded supermeshes will use their ENUM_TO_STR name to be fetched
        inline std::shared_ptr<const Supermesh> fetch_supermesh(BasicSupermeshType id)
        { return g_modelManager->fetch_supermesh(id); }

        // Clear cache of all models not used anywhere in the Cosmos
        // Shared pointers will remain with users whos already fetched them
        inline void clear_unused() { g_modelManager->clear_unused(); }

        // Clear ALL assets from cache
        // Shared pointers will remain with users whos already fetched them
        inline void clear_all() { g_modelManager->clear_all(); }
    }
}

#endif // MODEL_CACHE_H