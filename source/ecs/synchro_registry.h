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
        std::shared_ptr<T> register_synchro(Cosmos* ownerCosmos);

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
        std::unordered_map<const char*, std::shared_ptr<ISynchro>>& get_synchros_ref();

    private:
        // synchro typeid -> synchros desired entity signature
        std::unordered_map<const char*, Signature> m_signatures{};
        
        // synchro typeid -> synchro pointer
        // shares pointer between caller and registry so that it can be called externally
        std::unordered_map<const char*, std::shared_ptr<ISynchro>> m_synchros{};
    };
    

    template<typename T>
    std::shared_ptr<T> SynchroRegistry::register_synchro(Cosmos* ownerCosmos)
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
        // typeid & ISynchro pointer
        for (auto const& pair : m_synchros)
        {
            std::shared_ptr<ISynchro> const& synchro = pair.second;

            synchro->m_entities.erase(entity);
        }
    }

    inline void SynchroRegistry::change_entity_signature(Entity entity, Signature entitySign)
    {
        // typeid & ISynchro pointer
        for (auto const& pair : m_synchros)
        {
            const char* const& synchroTypeid = pair.first;
            std::shared_ptr<ISynchro> const& synchro = pair.second;
            Signature const& synchroSign = m_signatures[synchroTypeid];

            // add to synchro's known entities if it matches signatures
            // match -> entitySign equal to or superset of synchroSign
            if ((entitySign & synchroSign) == synchroSign)
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
    
    inline std::unordered_map<const char*, std::shared_ptr<ISynchro>>& SynchroRegistry::get_synchros_ref()
    {
        return m_synchros;
    }
}

#endif // SYNCHRO_REGISTRY_H