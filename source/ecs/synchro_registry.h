#ifndef SYNCHRO_REGISTRY_H
#define SYNCHRO_REGISTRY_H

//#include "intercession_pch.h"
#include <memory>
#include <typeinfo>
#include <unordered_map>

#include "ecs_types.h"
#include "i_synchro.h"
#include "logging/pleep_log.h"


namespace pleep
{
    // forward declare Cosmos to pass through to registered Synchros
    class Cosmos;

    class SynchroRegistry
    {
    public:
        // create AND register synchro for given templated synchro type
        // return created type
        template<typename T>
        std::shared_ptr<T> register_synchro(std::shared_ptr<Cosmos> ownerCosmos);

        // find synchro based on template and overwrite its desired signature
        // this does NOT recalculate its entities accordingly
        template<typename T>
        void set_signature(Signature sign);
        
        // erase a destroyed entity from all synchros
        void clear_entity(Entity entity);

        // re-determine which synchros have this entity in their working set
        void change_entity_signature(Entity entity, Signature entitySign);

        // return reference to internal synchro map
        // this is potentially dangerous, but cosmos needs access to iterate
        std::unordered_map<const char*, std::shared_ptr<I_Synchro>>& get_synchros_ref();

        // Return list of synchro typeid names
        // Not ordered, though it should not matter
        std::vector<std::string> stringify();

    private:
        // synchro typeid -> synchros desired entity signature
        std::unordered_map<const char*, Signature> m_signatures{};
        
        // synchro typeid -> synchro pointer
        // shares pointer between caller and registry so that it can be called externally
        std::unordered_map<const char*, std::shared_ptr<I_Synchro>> m_synchros{};
    };
    

    template<typename T>
    std::shared_ptr<T> SynchroRegistry::register_synchro(std::shared_ptr<Cosmos> ownerCosmos)
    {
        const char* typeName = typeid(T).name();

        if (m_synchros.find(typeName) != m_synchros.end())
        {
            PLEEPLOG_ERROR("Cannot register synchro " + std::string(typeName) + " which is already registered");
            throw std::runtime_error("SynchroRegistry cannot register synchro " + std::string(typeName) + " which is already registered");
        }

        std::shared_ptr<T> synchro = std::make_shared<T>(ownerCosmos);
        m_synchros.insert({typeName, synchro});
        return synchro;
    }

    template<typename T>
    void SynchroRegistry::set_signature(Signature sign)
    {
        const char* typeName = typeid(T).name();

        if (m_synchros.find(typeName) == m_synchros.end())
        {
            PLEEPLOG_ERROR("Cannot set signature of synchro " + std::string(typeName) + " which has not been registered");
            throw std::runtime_error("SynchroRegistry cannot set signature of synchro " + std::string(typeName) + " which has not been registered");
        }

        m_signatures.insert({typeName, sign});
    }
    
    inline void SynchroRegistry::clear_entity(Entity entity)
    {
        // typeid & I_Synchro pointer
        for (auto const& pair : m_synchros)
        {
            std::shared_ptr<I_Synchro> const& synchro = pair.second;

            synchro->m_entities.erase(entity);
        }
    }

    inline void SynchroRegistry::change_entity_signature(Entity entity, Signature entitySign)
    {
        // typeid & I_Synchro pointer
        for (auto const& pair : m_synchros)
        {
            const char* const& synchroTypeid = pair.first;
            std::shared_ptr<I_Synchro> const& synchro = pair.second;
            Signature const& synchroSign = m_signatures[synchroTypeid];

            // add to synchro's known entities if it matches signatures
            // match -> entitySign equal to or superset of synchroSign
            if ((entitySign & synchroSign) == synchroSign && synchroSign.any())
            {
                synchro->m_entities.insert(entity);
            }
            // otherwise clear from synchros who may have had before
            else
            {
                synchro->m_entities.erase(entity);
            }
        }
    }
    
    inline std::unordered_map<const char*, std::shared_ptr<I_Synchro>>& SynchroRegistry::get_synchros_ref()
    {
        return m_synchros;
    }
    
    inline std::vector<std::string> SynchroRegistry::stringify()
    {
        std::vector<std::string> synchroNames;
        synchroNames.reserve(m_synchros.size());
        for (auto synchroIt : m_synchros)
        {
            synchroNames.push_back(synchroIt.first);
        }

        // sort alphabetically just for consistency?
        std::sort(synchroNames.begin(), synchroNames.end());

        // leave order as whatever map was in
        return synchroNames;
    }
}

#endif // SYNCHRO_REGISTRY_H