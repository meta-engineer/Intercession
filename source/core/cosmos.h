#ifndef COSMOS_H
#define COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "ecs/ecs_types.h"
#include "ecs/entity_registry.h"
#include "ecs/component_registry.h"
#include "ecs/synchro_registry.h"
#include "events/event_types.h"
#include "events/event_broker.h"

namespace pleep
{
    // Cosmos is simply a harness for an ecs
    // it is "everything that exists" in a simulated verse of some sort
    // all types of "scenes" (a title screen with mouse & text input, or a 3D world with a character controller)
    // should be able to fit in a generic ecs framework
    // the differentiation will come from how the Cosmos owner (CosmosContext) builds its registries
    // the specific types of compnents/synchros (and their dynamos) will determine how the cosmos manifests
    class Cosmos : public std::enable_shared_from_this<Cosmos>
    {
    public:
        // build empty ecs
        Cosmos(EventBroker* sharedBroker, const TimesliceId localTimesliceIndex = NULL_TIMESLICEID);
        ~Cosmos();

        // call all synchros to invoke entity updates
        // synchros have access to parent (me) to access components for their entities
        // and can be provided access (after registering) to a dynamo for
        //   external resources (like sending events)
        void update();

        // cosmos does not receive/know dynamos
        // context that registers synchros will receive returned synchros and
        // attach dynamos/config as necessary

        // ***** ECS methods *****

        // create & register empty entity hosted by us
        // If we are not a timeslice host (client) we have to create a temporary entity
        // with a null TimesliceId and signal to our current connected host to create a valid version to overwrite it
        Entity create_entity();

        // create empty entity & register it as the given value
        // Check Entity should be from another host?
        // Can there be an edge case where another host passes us our own entity?
        // Returns true when entity is valid to use and register components to
        bool register_entity(Entity entity);

        // remove entity & related components, and clear it from any synchros
        void destroy_entity(Entity entity);

        // forwards to EntityRegistry
        // return number of existing entities in this cosmos of any type
        size_t get_entity_count();
        
        // passthrough to EntityRegistry->get_signature()
        Signature get_entity_signature(Entity entity);

        // get reference to signature map for iterating
        std::unordered_map<Entity, Signature>& get_signatures_ref();

        // find which entity has this component
        // Linear complexity (with number of components of this type)
        // operator == must be defined for T
        // if components aren't unique this may be invalid
        // if component doesn't exist, returns NULL_ENTITY
        template<typename T>
        Entity find_entity(T component);

        // increase existence count of TemporalEntity that this Entity belongs to
        // throws error if count is currently zero (must be set by create/register)
        void increment_hosted_temporal_entity_count(Entity entity);
        // decrease existence count of TemporalEntity that this Entity belongs to
        // throws error if count is currently zero (must be set by create/register)
        void decrement_hosted_temporal_entity_count(Entity entity);
        // Returns total number of entities that share this entity's TemporalEntityId across the timeline
        // returns 0 if entity is not hosted by this timeslice
        size_t get_hosted_temporal_entity_count(Entity entity);
        // Returns total number of TampoeralEntities this cosmos is currently hosting
        size_t get_num_hosted_temporal_entities();

        // setup component T to be usable in this cosmos
        template<typename T>
        void register_component();

        // add an instance of a registered component T to a registered entity
        template<typename T>
        void add_component(Entity entity, T component);

        // add default constructed component to a registered entity
        void add_component(Entity entity, ComponentType componentId);

        // remove component T from entity, and update all syncrhos
        template<typename T>
        void remove_component(Entity entity);

        // find component T of entity
        template<typename T>
        T& get_component(Entity entity);

        // get registered component T's typeid
        template<typename T>
        ComponentType get_component_type();

        const char* get_component_name(ComponentType componentId);
        

        // setup synchro T to be usable in this cosmos
        // its signature will be set using ISynchro::get_signature
        // and return it once created
        // CAREFUL! Entity associations are only updated on individual component changes. Synchro will NOT be associated with any existing entities. Build synchros BEFORE adding entities/components
        template<typename T>
        std::shared_ptr<T> register_synchro();

        // TODO: Do we need a method to change a synchro signature after registry and then recalculate its entities accordingly?


        ///// Helper methods for sending entity information in Messages (for events or network) /////

        // Pack each component of entity into message in reverse (stacked) order
        // ***Does NOT include entity id and signature (header)
        // We have to specify Message<EventId> becuase IComponentArray must use virtual methods
        void serialize_entity_components(Entity entity, EventMessage& msg);

        // Unpack each component according to entity signature
        // ***Does NOT include entity id and signature (header)
        void Cosmos::deserialize_entity_components(Entity entity, EventMessage& msg);

        // Unpack a component (specified by name) from message and update entity with its new values
        // Use if you want to manually unpack each component and validate them before writing
        // *** compnentName MUST BE THE pointer from typeid!!! USE get_component_name()!!!
        // ***See deserialize_entity_components for template
        void deserialize_and_write_component(Entity entity, const char* componentName, EventMessage& msg);
        
        // Ordered vector of all synchro typeid names
        std::vector<std::string> stringify_synchro_registry();

        // Ordered vector of all synchro typeid names
        std::vector<std::string> stringify_component_registry();

    private:
        // use ECS (Entity, Component, Synchro) pattern to optimize update calls
        std::unique_ptr<ComponentRegistry> m_componentRegistry;
        std::unique_ptr<EntityRegistry>    m_entityRegistry;
        std::unique_ptr<SynchroRegistry>   m_synchroRegistry;

        // for emitting events::cosmos
        EventBroker* m_sharedBroker = nullptr;
    };


    // ***** ECS methods *****
    // templates need to be accessable to all translation units that use Cosmos
    // (non-template entity methods also provided as inline for organization)
    
    inline Entity Cosmos::create_entity() 
    {
        Entity entity = m_entityRegistry->create_entity();

        // broadcast that new entity exists
        EventMessage newEntityEvent(events::cosmos::ENTITY_CREATED);
        events::cosmos::ENTITY_CREATED_params newEntityParams {
            entity
        };
        newEntityEvent << newEntityParams;
        m_sharedBroker->send_event(newEntityEvent);

        return entity;
    }
    
    inline bool Cosmos::register_entity(Entity entity)
    {
        bool isEntityValid = m_entityRegistry->register_entity(entity);

        if (!isEntityValid) return false;

        // broadcast that new entity exists
        EventMessage newEntityEvent(events::cosmos::ENTITY_CREATED);
        events::cosmos::ENTITY_CREATED_params newEntityParams {
            entity
        };
        newEntityEvent << newEntityParams;
        m_sharedBroker->send_event(newEntityEvent);

        return true;
    }
    
    inline void Cosmos::destroy_entity(Entity entity) 
    {
        m_entityRegistry->destroy_entity(entity);
        m_componentRegistry->clear_entity(entity);
        m_synchroRegistry->clear_entity(entity);

        // broadcast entity no longer exists
        EventMessage removedEntityEvent(events::cosmos::ENTITY_REMOVED);
        events::cosmos::ENTITY_REMOVED_params removedEntityParams {
            entity
        };
        removedEntityEvent << removedEntityParams;
        m_sharedBroker->send_event(removedEntityEvent);
    }
    
    inline size_t Cosmos::get_entity_count() 
    {
        return m_entityRegistry->get_entity_count();
    }

    inline Signature Cosmos::get_entity_signature(Entity entity)
    {
        return m_entityRegistry->get_signature(entity);
    }
    
    inline std::unordered_map<Entity, Signature>& Cosmos::get_signatures_ref()
    {
        return m_entityRegistry->get_signatures_ref();
    }
    
    template<typename T>
    Entity Cosmos::find_entity(T component)
    {
        return m_componentRegistry->find_entity(component);
    }
    
    inline void Cosmos::increment_hosted_temporal_entity_count(Entity entity)
    {
        m_entityRegistry->increment_hosted_temporal_entity_count(entity);
    }
    inline void Cosmos::decrement_hosted_temporal_entity_count(Entity entity)
    {
        m_entityRegistry->decrement_hosted_temporal_entity_count(entity);
    }
    inline size_t Cosmos::get_hosted_temporal_entity_count(Entity entity)
    {
        return m_entityRegistry->get_hosted_temporal_entity_count(entity);
    }
    inline size_t Cosmos::get_num_hosted_temporal_entities()
    {
        return m_entityRegistry->get_num_hosted_temporal_entities();
    }

    template<typename T>
    void Cosmos::register_component() 
    {
        m_componentRegistry->register_component_type<T>();
    }
    
    template<typename T>
    void Cosmos::add_component(Entity entity, T component) 
    {
        m_componentRegistry->add_component<T>(entity, component);

        // flip signature bit for this component
        Signature sign = m_entityRegistry->get_signature(entity);
        sign.set(m_componentRegistry->get_component_type<T>(), true);
        m_entityRegistry->set_signature(entity, sign);

        m_synchroRegistry->change_entity_signature(entity, sign);
    }

    inline void Cosmos::add_component(Entity entity, ComponentType componentId)
    {
        m_componentRegistry->add_component(entity, componentId);

        // flip signature bit for this component
        Signature sign = m_entityRegistry->get_signature(entity);
        sign.set(componentId, true);
        m_entityRegistry->set_signature(entity, sign);

        m_synchroRegistry->change_entity_signature(entity, sign);
    }
    
    template<typename T>
    void Cosmos::remove_component(Entity entity) 
    {
        m_componentRegistry->remove_component<T>(entity);
        
        // flip signature bit for this component
        Signature sign = m_entityRegistry->get_signature(entity);
        sign.set(m_componentRegistry->get_component_type<T>(), false);
        m_entityRegistry->set_signature(entity, sign);

        m_synchroRegistry->change_entity_signature(entity, sign);
    }
    
    template<typename T>
    T& Cosmos::get_component(Entity entity) 
    {
        return m_componentRegistry->get_component<T>(entity);
    }
    
    template<typename T>
    ComponentType Cosmos::get_component_type() 
    {
        return m_componentRegistry->get_component_type<T>();
    }
    
    inline const char* Cosmos::get_component_name(ComponentType componentId) 
    {
        return m_componentRegistry->get_component_name(componentId);
    }
    
    template<typename T>
    std::shared_ptr<T> Cosmos::register_synchro() 
    {
        std::shared_ptr<T> newSynchro = m_synchroRegistry->register_synchro<T>(shared_from_this());

        // we'll expect T to be ISynchro
        m_synchroRegistry->set_signature<T>(newSynchro->derive_signature(shared_from_this()));

        return newSynchro;
    }
}

#endif // COSMOS_H