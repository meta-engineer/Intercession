#ifndef COSMOS_H
#define COSMOS_H

//#include "intercession_pch.h"
#include <memory>

#include "ecs/ecs_types.h"
#include "ecs/entity_registry.h"
#include "ecs/component_registry.h"
#include "ecs/synchro_registry.h"
#include "events/event_types.h"

namespace pleep
{
    // Cosmos is simply a harness for an ecs
    // it is "everything that exists" in a simulated verse of some sort
    // all types of "scenes" (a title screen with mouse & text input, or a 3D world with a character controller)
    // should be able to fit in a generic ecs framework
    // the differentiation will come from how the Cosmos owner (CosmosContext) builds its registries
    // the specific types of compnents/synchros (and their dynamos) will determine how the cosmos manifests
    class Cosmos
    {
    public:
        // build empty ecs
        Cosmos();
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

        // register empty entity
        Entity create_entity();

        // remove entity & related components, and clear it from any synchros
        void destroy_entity(Entity entity);

        // foreward count fetch to EntityRegistry
        Entity get_entity_count();
        
        // passthrough to EntityRegistry->get_signature()
        Signature get_entity_signature(Entity entity);

        // find which entity has this component
        // Linear complexity (with number of components of this type)
        // operator == must be defined for T
        // if components aren't unique this may be invalid
        // if component doesn't exist, returns NULL_ENTITY
        template<typename T>
        Entity find_entity(T component);

        // setup component T to be usable in this cosmos
        template<typename T>
        void register_component();

        // add an instance of a registered component T to a registered entity
        template<typename T>
        void add_component(Entity entity, T component);

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
        // and return it once created
        template<typename T>
        std::shared_ptr<T> register_synchro();

        // set synchro T to have entity signature sign.
        // empty sign will not recieve any entities
        // CAREFUL! this does NOT recalculate its entities accordingly! Build synchros BEFORE components
        template<typename T>
        void set_synchro_signature(Signature sign);


        ///// Helper methods for sending entity information in Messages (for events or network) /////

        // Pack each component of entity into message in reverse (stacked) order
        // ***Does NOT include timeline id and signature (header)
        // We have to specify Message<EventId> becuase IComponentArray must use virtual methods
        void serialize_entity_components(Entity entity, EventMessage msg);

        // Unpack a component (specified by name) from message and update entity with its new values
        void deserialize_and_write_component(Entity entity, std::string componentName, EventMessage msg);

    private:
        // use ECS (Entity, Component, Synchro) pattern to optimize update calls
        std::unique_ptr<ComponentRegistry> m_componentRegistry;
        std::unique_ptr<EntityRegistry>    m_entityRegistry;
        std::unique_ptr<SynchroRegistry>   m_synchroRegistry;

        // ECS synchros know their respective dynamos and feed components into them on update
        // synchros are given a dynamo to attach to by register_synchro caller (context)
        // deleting or significantly mutating a dynamo must apply to any synchros attached to it
        //   to avoid a synchro dereferencing an invalid dynamo
        
        // synchros are dynamically created by CosmosContext
        //   (it will need some input "scene" file to know what to do)

        // examples of synchros are:
        // RenderSynchro: submits renderable components to a dynamo that accesses a window api
        // ControlSynchro: submits components which receive input to a dynamo that accesses a window api
        // PhysicsSynchro: submits components with physical properties to a dynmo that does motion integration/collision
        // AudioSynchro: submits audable/playable components to a dynamo that accesses an audio api
        // NetSynchro: submits components which are remotely synchronized to a dynamo that accesses a network api
    };


    // ***** ECS methods *****
    // templates need to be accessable to all translation units that use Cosmos
    // (non-template entity methods also provided as inline for organization)
    
    inline Entity Cosmos::create_entity() 
    {
        return m_entityRegistry->create_entity();
    }
    
    inline void Cosmos::destroy_entity(Entity entity) 
    {
        m_entityRegistry->destroy_entity(entity);
        m_componentRegistry->clear_entity(entity);
        m_synchroRegistry->clear_entity(entity);
    }
    
    inline Entity Cosmos::get_entity_count() 
    {
        return m_entityRegistry->get_entity_count();
    }

    inline Signature Cosmos::get_entity_signature(Entity entity)
    {
        return m_entityRegistry->get_signature(entity);
    }
    
    template<typename T>
    Entity Cosmos::find_entity(T component)
    {
        return m_componentRegistry->find_entity(component);
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
        return m_synchroRegistry->register_synchro<T>(this);
    }
    
    template<typename T>
    void Cosmos::set_synchro_signature(Signature sign) 
    {
        m_synchroRegistry->set_signature<T>(sign);
    }
}

#endif // COSMOS_H