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
#include "spacetime/timestream_state.h"

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
        // receive broker from context
        // receive timeslice ID from networker
        Cosmos(std::shared_ptr<EventBroker> sharedBroker, const TimesliceId localTimesliceIndex = NULL_TIMESLICEID);
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
        // isTemporal: sets entity chainlink to be null, and it won't participate in any time travel/superpositions
        // source: can specify an entity creation was triggered by.
        //   Used to avoid duplicate creations by clients or child servers
        //   NULL_ENTITY always allows creation
        Entity create_entity(bool isTemporal = true, Entity source = NULL_ENTITY);

        // create empty entity & register it as the given value
        // Check Entity should be from another host?
        // Can there be an edge case where another host passes us our own entity?
        // Returns true when entity is valid to use and register components to
        // initialize all components in initSign before sending creation event
        bool register_entity(Entity entity, Signature initSign = {}, Entity source = NULL_ENTITY);

        // flag entity to be destroyed before next update cycle
        // source: can specify an entity deletion was triggered by.
        //   Used to avoid duplicate deletions by clients or child servers
        //   NULL_ENTITY always allows deletion
        void condemn_entity(Entity entity, Entity source = NULL_ENTITY);

        // forwards to EntityRegistry
        // return number of existing entities in this cosmos of any type
        size_t get_entity_count();
        
        // passthrough to EntityRegistry->get_signature()
        // returns empty signature if entity does not exist (or it is actually empty)
        Signature get_entity_signature(Entity entity);

        // returns true if this entity exists in this cosmos (exact match)
        bool entity_exists(Entity entity);

        // get reference to signature map for iterating
        std::unordered_map<Entity, Signature>& get_signatures_ref();

        // find which entity has this component
        // Linear complexity (with number of components of this type)
        // operator== must be defined for T
        // if components aren't unique this may be invalid
        // if component doesn't exist, returns NULL_ENTITY
        template<typename T>
        Entity find_entity(T component);

        // increase existence count of TemporalEntity that this Entity belongs to
        // throws error if count is currently zero (must be set by create/register)
        void increment_hosted_entity_count(Entity entity);
        // decrease existence count of TemporalEntity that this Entity belongs to
        // throws error if count is currently zero (must be set by create/register)
        void decrement_hosted_entity_count(Entity entity);
        // Returns total number of entities that share this entity's TemporalEntityId across the timeline
        // returns 0 if entity is not hosted by this timeslice
        size_t get_hosted_entity_count(Entity entity);
        // Returns total number of TampoeralEntities this cosmos is currently hosting
        size_t get_num_hosted_entities();

        // setup component T to be usable in this cosmos
        // Assign category to the registered type
        template<typename T>
        void register_component(ComponentCategory category = ComponentCategory::downstream);

        // Returns signature of Cosmos' components only belonging to category
        Signature get_category_signature(ComponentCategory category);

        // add an instance of a registered component T to a registered entity
        template<typename T>
        void add_component(Entity entity, T component);

        // add default constructed component to a registered entity
        void add_component(Entity entity, ComponentType componentId);

        // remove component T from entity, and update all syncrhos
        template<typename T>
        void remove_component(Entity entity);

        // remove component of id from entity, and update all synchros
        void remove_component(Entity entity, ComponentType componentId);

        // safely determine if entity has component of type T
        template<typename T>
        bool has_component(Entity entity);
        
        // safely determine if entity has component with id
        bool has_component(Entity entity, ComponentType componentId);

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

        // Pack each component of entity in sign into msg in reverse (stacked) order
        // Components in entity signature but missing from sign are ignored.
        // Components in sign but missing from entity signature will THROW range_error
        // !!! Does NOT include entity id and signature (header)
        // (We have to specify Message<EventId> because IComponentArray must use virtual methods)
        void serialize_entity_components(Entity entity, Signature sign, EventMessage& msg);

        // Unpack each component from msg according to entity signature
        // Only components in category are written (and added if necessary)
        // If ComponentCategory::all, then components are also removed from entity to match sign
        // !!! msg must NOT include header params (entity id and signature)
        void deserialize_entity_components(Entity entity, Signature sign, EventMessage& msg, ComponentCategory category = ComponentCategory::all);

        // Unpack a component (specified by type) from message and update entity with its new values
        // Use if you want to manually unpack each component and validate them before writing
        // ***See deserialize_entity_components for template
        void deserialize_single_component(Entity entity, ComponentType type, EventMessage& msg);

        // leverage same component dispatching to remove data from a message
        // Unpack the component from message and just forget it
        // Use to ignore certain ComponentCategories
        void discard_single_component(ComponentType type, EventMessage& msg);

        // Set shared "special" entity for this cosmos instance
        // returns false if entity does not exist
        bool set_focal_entity(Entity entity);
        // If entity no longer exists since last set/get, sets to NULL_ENITTY before return
        Entity get_focal_entity(); 

        // increments coherency counter for state
        // increment time should reflect the primary update cycle of the simulation
        void increment_coherency();
        uint16_t get_coherency();
        // Jump discontinuously to a new coherency point
        // make sure you know what you're doing with this!
        void set_coherency(uint16_t gotoCoherency);

        TimesliceId get_host_id();

        // set state of entity annotated with current coherency
        // returns false if entity does not exist
        bool set_timestream_state(Entity entity, TimestreamState newState);
        // return state and coherency when set was called
        // returns merged, 0 if entity did not exist
        std::pair<TimestreamState, uint16_t> get_timestream_state(Entity entity);

        // defers entity id management to given cosmos, registers it locally instead
        // owns shared pointer keeping cosmos alive
        // pass nullptr to un-link
        void link_cosmos(std::shared_ptr<Cosmos> sourceCosmos = nullptr);
        
        // Ordered vector of all synchro typeid names
        std::vector<std::string> stringify_synchro_registry();

        // Ordered vector of all synchro typeid names
        std::vector<std::string> stringify_component_registry();

    private:
        // remove entity & related components, and clear it from any synchros
        // angerous if references have been submitted to dynamos
        void destroy_entity(Entity entity, Entity source = NULL_ENTITY);

        // event handlers
        void _condemn_all_handler(EventMessage condemnEvent);

        // use ECS (Entity, Component, Synchro) pattern to optimize update calls
        std::unique_ptr<ComponentRegistry> m_componentRegistry;
        std::unique_ptr<EntityRegistry>    m_entityRegistry;
        std::unique_ptr<SynchroRegistry>   m_synchroRegistry;

        // for emitting events::cosmos
        std::shared_ptr<EventBroker> m_sharedBroker = nullptr;

        // Barycenter of the cosmos, stored centrally for dynamos to coordinate
        // used by client for network to send updates, camera targetting, and input synchro dispatching
        Entity m_focalEntity = NULL_ENTITY;

        // rolling counter of Cosmos state updates
        // incremented by the Context who owns us when dynamos are invoked
        // Context decides at what point we have meaningfully changed to the next state
        // (not necessarily the number of update() calls, because simulation/render updates are not in lockstep)
        uint16_t m_stateCoherency = 0;

        // some behavior changes depending on if this Cosmos is part of a client
        // or if we are operating on entities which we also host
        // (clients have NULL_TIMESLICEID)
        TimesliceId m_hostId = NULL_TIMESLICEID;

        // Set containing entities who were requested to be deleted.
        // and the source of their removal
        // They will be deleted directly before next update,
        // so all component references should be cleared by then
        std::set<std::pair<Entity, Entity>> m_condemned;

        // store TimestreamState information locally so it is not propogated into the past
        // uint16_t is "time"stamp for last update to this entity's state
        // No entry implies TimestreamState::merged
        std::unordered_map<Entity, std::pair<TimestreamState, uint16_t>> m_timestreamStates;
        /// TODO: how do we inform clients of timestream states... if any? Send updates upon parallel extraction?

        // if not nullptr, defer entity id creation to this cosmos instead
        std::shared_ptr<Cosmos> m_linkedCosmos = nullptr;
    };


    // ***** ECS methods *****
    // templates need to be accessable to all translation units that use Cosmos
    // (non-template entity methods also provided as inline for organization)
    
    inline Entity Cosmos::create_entity(bool isTemporal, Entity source) 
    {
        if (m_linkedCosmos != nullptr)
        {
            // threadsafe?
            Entity deferredEntity = m_linkedCosmos->create_entity(isTemporal, NULL_ENTITY);
            PLEEPLOG_DEBUG("Defferred entity creation to produce: " + std::to_string(deferredEntity));
            if (this->register_entity(deferredEntity, {}, source))
            {
                return deferredEntity;
            }

            return NULL_ENTITY;
        }
        // don't proceed if we deferred to a linked cosmos

        // create entity in cosmos if either:
        //  - source is NULL (meaning creation is forced to happen)
        //  - source is link=0 and we're on a server
        //  - source is forked
        if (source == NULL_ENTITY ||
            (derive_causal_chain_link(source) == 0 && m_hostId != NULL_TIMESLICEID) ||
            (is_divergent(get_timestream_state(source).first)))
        {
            // proceed
        }
        else
        {
            return NULL_ENTITY;
        }

        // starting link value depends on parameters
        /// NOTE: this means nonTemporal entities can only create nonTemporal entities
        CausalChainlink link = !isTemporal ? NULL_CAUSALCHAINLINK :
                                m_hostId != NULL_TIMESLICEID ? m_hostId :
                                source != NULL_ENTITY ? derive_causal_chain_link(source) :
                                0;

        Entity entity = m_entityRegistry->create_entity(link);

        // broadcast that new entity exists
        EventMessage newEntityEvent(events::cosmos::ENTITY_CREATED, m_stateCoherency);
        events::cosmos::ENTITY_CREATED_params newEntityParams {
            entity,
            {},
            source
        };
        newEntityEvent << newEntityParams;
        m_sharedBroker->send_event(newEntityEvent);

        return entity;
    }
    
    inline bool Cosmos::register_entity(Entity entity, Signature initSign, Entity source)
    {
        if (entity == NULL_ENTITY) return false;

        bool isEntityValid = m_entityRegistry->register_entity(entity);

        if (!isEntityValid) return false;

        // entity is created successfully, init its components:
        for (ComponentType c = 0; c < initSign.size(); c++)
        {
            if (initSign.test(c)) add_component(entity, c);
        }

        // broadcast that new entity exists
        EventMessage newEntityEvent(events::cosmos::ENTITY_CREATED, m_stateCoherency);
        events::cosmos::ENTITY_CREATED_params newEntityParams {
            entity,
            initSign,
            source
        };
        newEntityEvent << newEntityParams;
        m_sharedBroker->send_event(newEntityEvent);

        return true;
    }

    inline void Cosmos::condemn_entity(Entity entity, Entity source)
    {
        if (entity == NULL_ENTITY) return;

        PLEEPLOG_TRACE("Entity " + std::to_string(entity) + " was condemned to deletion.");
        m_condemned.insert({ entity, source });
    }
    
    // private:
    inline void Cosmos::destroy_entity(Entity entity, Entity source) 
    {
        if (entity == NULL_ENTITY) return;

        if (!m_entityRegistry->destroy_entity(entity))
        {
            // Entity may not have existed, allow impodent calls
            return;
        }
        m_componentRegistry->clear_entity(entity);
        m_synchroRegistry->clear_entity(entity);

        PLEEPLOG_TRACE("Entity " + std::to_string(entity) + " was destroyed");

        // clear focal entity if we just deleted it
        if (entity == m_focalEntity) set_focal_entity(NULL_ENTITY);
        // clear timestream state if it has one
        m_timestreamStates.erase(entity);

        // Entity will be re-added to availability queue once it's host count decrements to 0
        // we will message to our NetworkDynamo to decrement OTHER hosts
        //     (event signalled by cosmos after this returns)
        // but clients (and servers) will decrement themselves immediately here
        if (m_hostId == derive_timeslice_id(entity))
        {
            decrement_hosted_entity_count(entity);
        }

        // broadcast entity no longer exists
        EventMessage removedEntityEvent(events::cosmos::ENTITY_REMOVED, m_stateCoherency);
        events::cosmos::ENTITY_REMOVED_params removedEntityParams {
            entity,
            source
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
    
    inline bool Cosmos::entity_exists(Entity entity)
    {
        return m_entityRegistry->get_signatures_ref().count(entity) != 0;
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
    
    inline void Cosmos::increment_hosted_entity_count(Entity entity)
    {
        m_entityRegistry->increment_hosted_entity_count(entity);
    }
    inline void Cosmos::decrement_hosted_entity_count(Entity entity)
    {
        m_entityRegistry->decrement_hosted_entity_count(entity);
    }
    inline size_t Cosmos::get_hosted_entity_count(Entity entity)
    {
        return m_entityRegistry->get_hosted_entity_count(entity);
    }
    inline size_t Cosmos::get_num_hosted_entities()
    {
        return m_entityRegistry->get_num_hosted_entities();
    }

    template<typename T>
    void Cosmos::register_component(ComponentCategory category) 
    {
        m_componentRegistry->register_component_type<T>(category);
    }

    inline Signature Cosmos::get_category_signature(ComponentCategory category)
    {
        if (ComponentCategory::all == category)
        {
            Signature all; all.set();
            return all;
        }
        return m_componentRegistry->get_category_signature(category);
    }
    
    template<typename T>
    void Cosmos::add_component(Entity entity, T component) 
    {
        if (entity == NULL_ENTITY) return;

        m_componentRegistry->add_component<T>(entity, component);

        // flip signature bit for this component
        Signature sign = m_entityRegistry->get_signature(entity);
        sign.set(m_componentRegistry->get_component_type<T>(), true);
        m_entityRegistry->set_signature(entity, sign);
        m_synchroRegistry->change_entity_signature(entity, sign);
    }

    inline void Cosmos::add_component(Entity entity, ComponentType componentId)
    {
        if (entity == NULL_ENTITY) return;

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
        if (entity == NULL_ENTITY) return;

        m_componentRegistry->remove_component<T>(entity);
        
        // flip signature bit for this component
        Signature sign = m_entityRegistry->get_signature(entity);
        sign.set(m_componentRegistry->get_component_type<T>(), false);
        m_entityRegistry->set_signature(entity, sign);
        m_synchroRegistry->change_entity_signature(entity, sign);
    }
    
    inline void Cosmos::remove_component(Entity entity, ComponentType componentId)
    {
        if (entity == NULL_ENTITY) return;

        m_componentRegistry->remove_component(entity, componentId);
        
        // flip signature bit for this component
        Signature sign = m_entityRegistry->get_signature(entity);
        sign.set(componentId, false);
        m_entityRegistry->set_signature(entity, sign);
        m_synchroRegistry->change_entity_signature(entity, sign);
    }

    template<typename T>
    bool Cosmos::has_component(Entity entity)
    {
        if (entity == NULL_ENTITY) return false;

        return m_componentRegistry->has_component(entity, m_componentRegistry->get_component_type<T>());
    }
    
    inline bool Cosmos::has_component(Entity entity, ComponentType componentId)
    {
        if (entity == NULL_ENTITY) return false;

        return m_componentRegistry->has_component(entity, componentId);
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
        m_synchroRegistry->set_signature<T>(newSynchro->derive_signature());

        return newSynchro;
    }
}

#endif // COSMOS_H