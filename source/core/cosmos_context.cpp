#include "cosmos_context.h"

#include "logging/pleep_log.h"

// TODO: This is temporary for building hard-coded entities
#include "controlling/camera_control_synchro.h"
#include "controlling/physics_control_synchro.h"
#include "rendering/render_synchro.h"
#include "rendering/lighting_synchro.h"
#include "physics/physics_synchro.h"
#include "physics/box_collider_synchro.h"

#include "physics/transform_component.h"
#include "physics/physics_component.h"
#include "physics/box_collider_component.h"
#include "physics/rigid_body_component.h"
#include "controlling/physics_control_component.h"
#include "controlling/camera_control_component.h"
#include "rendering/model_component.h"
#include "rendering/model_builder.h"
#include "rendering/camera_component.h"
#include "rendering/light_source_component.h"
#include "ecs/tag_component.h"

namespace pleep
{
    CosmosContext::CosmosContext()
        : m_currentCosmos(nullptr)
        , m_renderDynamo(nullptr)
        , m_controlDynamo(nullptr)
        , m_physicsDynamo(nullptr)
    {
        m_running = false;

        // build event listener
        m_eventBroker = new EventBroker();
        // setup context's "fallback" listeners
        m_eventBroker->add_listener(METHOD_LISTENER(events::window::QUIT, CosmosContext::_quit_handler));
    }
    
    CosmosContext::CosmosContext(GLFWwindow* windowApi)
        : CosmosContext()
    {
        // Broker is ready to be distributed

        // construct dynamos
        m_renderDynamo  = new RenderDynamo(m_eventBroker, windowApi);
        m_controlDynamo = new ControlDynamo(m_eventBroker, windowApi);
        m_physicsDynamo = new PhysicsDynamo(m_eventBroker);
        
        // build empty starting cosmos
        m_currentCosmos = new Cosmos();

        // populate starting cosmos
        // eventually we'll pass some config param here
        _build_cosmos();
    }
    
    CosmosContext::~CosmosContext()
    {
        // delete cosmos first to avoid null dynamo dereferences
        delete m_currentCosmos;

        delete m_physicsDynamo;
        delete m_controlDynamo;
        delete m_renderDynamo;

        delete m_eventBroker;
    }
    
    void CosmosContext::run()
    {
        m_running = true;

        // main game loop
        PLEEPLOG_TRACE("Starting \"main loop\"");
        double lastTimeVal = glfwGetTime();
        double thisTimeVal;
        double deltaTime;

        while (m_running)
        {
            // ***** Init Frame *****
            // smarter way to get dt?
            thisTimeVal = glfwGetTime();
            deltaTime = thisTimeVal - lastTimeVal;
            lastTimeVal = thisTimeVal;

            // ***** Cosmos Update *****
            // invokes all registered synchros to process their entities
            m_currentCosmos->update();

            // flush dynamos of all synchro submissions
            m_controlDynamo->run_relays(deltaTime);
            m_physicsDynamo->run_relays(deltaTime);
            m_renderDynamo->run_relays(deltaTime);

            // ***** Post Processing *****
            // top ui layer in context for debug
            // TODO: abstract this to ui layer
            // TODO: implment some fetchable stats in Dynamos to show here
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            bool checkbox;
            // show ui window
            {
                static float f = 0.0f;
                static int counter = 0;

                // Create a window and append into it.
                ImGui::Begin("Context Debug");

                // Display some text (you can use a format strings too)
                ImGui::Text("This window runs above the cosmos");

                // Report Cosmos info
                std::string countString = "Entity Count: " + std::to_string(m_currentCosmos->get_entity_count());
                ImGui::Text(countString.c_str());

                // Edit bools storing our window open/close state
                ImGui::Checkbox("checkbox", &checkbox);

                // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
                //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                // Buttons return true when clicked (most widgets return true when edited/activated)
                if (ImGui::Button("Button"))
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::End();
            }
            // Render out ui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            

            // ***** Finish Frame *****
            // Context gets last word on any final superceding actions
            // Maybe IDynamo should have a generic "flush" method
            //   and we invoke all dynamos (to avoid having to specify)
            
            // RenderDynamo calls swap buffers
            m_renderDynamo->flush_frame();
        }
        
        PLEEPLOG_TRACE("Exiting \"main loop\"");
        // any non-destructor cleanup?
    }
    
    void CosmosContext::stop()
    {
        m_running = false;
    }
    
    void CosmosContext::_build_cosmos()
    {
        // we need to build synchros and link them with dynamos
        // until we can load from file we can manually call methods to build entities in its ecs
        // use imgui input in main loop do add more at runtime
        
        // register components
        m_currentCosmos->register_component<TransformComponent>();
        m_currentCosmos->register_component<CameraControlComponent>();
        m_currentCosmos->register_component<PhysicsControlComponent>();
        m_currentCosmos->register_component<ModelComponent>();
        m_currentCosmos->register_component<CameraComponent>();
        m_currentCosmos->register_component<LightSourceComponent>();
        m_currentCosmos->register_component<PhysicsComponent>();
        m_currentCosmos->register_component<BoxColliderComponent>();
        m_currentCosmos->register_component<RigidBodyComponent>();
        m_currentCosmos->register_component<SpringBodyComponent>();
        // register tag component as a normal component
        m_currentCosmos->register_component<TagComponent>();

        // register/create synchros, set component signatures
        // we shouldn't need to keep synchro references after we config them here, 
        // we'll only access through Cosmos
        // any other functionallity should be in dynamos
        PLEEPLOG_TRACE("Create Synchros");
        // TODO: get synchros to maintain their own signatures to fetch for registration

        std::shared_ptr<CameraControlSynchro> cameraControlSynchro = m_currentCosmos->register_synchro<CameraControlSynchro>();
        {
            cameraControlSynchro->attach_dynamo(m_controlDynamo);
            m_currentCosmos->set_synchro_signature<CameraControlSynchro>(CameraControlSynchro::get_signature(m_currentCosmos));
        }
        
        std::shared_ptr<PhysicsControlSynchro> physicsControlSynchro = m_currentCosmos->register_synchro<PhysicsControlSynchro>();
        {
            physicsControlSynchro->attach_dynamo(m_controlDynamo);
            m_currentCosmos->set_synchro_signature<PhysicsControlSynchro>(PhysicsControlSynchro::get_signature(m_currentCosmos));
        }

        // synchros are in a map so it isn't guarenteed that LightingSynchro is invoked before RenderSynchro
        // TODO: ordering of synchros in unordered_map DOES AFFECT run order, with undefined, NON-DETERMINISTIC behaviour
        std::shared_ptr<LightingSynchro> lightingSynchro = m_currentCosmos->register_synchro<LightingSynchro>();
        {
            lightingSynchro->attach_dynamo(m_renderDynamo);
            m_currentCosmos->set_synchro_signature<LightingSynchro>(LightingSynchro::get_signature(m_currentCosmos));
        }

        std::shared_ptr<RenderSynchro> renderSynchro = m_currentCosmos->register_synchro<RenderSynchro>();
        {
            renderSynchro->attach_dynamo(m_renderDynamo);
            m_currentCosmos->set_synchro_signature<RenderSynchro>(RenderSynchro::get_signature(m_currentCosmos));
        }

        // TODO: maybe specify this is "motion integration" not just all physics
        std::shared_ptr<PhysicsSynchro> physicsSynchro = m_currentCosmos->register_synchro<PhysicsSynchro>();
        {
            physicsSynchro->attach_dynamo(m_physicsDynamo);
            m_currentCosmos->set_synchro_signature<PhysicsSynchro>(PhysicsSynchro::get_signature(m_currentCosmos));
        }

        std::shared_ptr<BoxColliderSynchro> boxColliderSynchro = m_currentCosmos->register_synchro<BoxColliderSynchro>();
        {
            boxColliderSynchro->attach_dynamo(m_physicsDynamo);
            m_currentCosmos->set_synchro_signature<BoxColliderSynchro>(BoxColliderSynchro::get_signature(m_currentCosmos));
        }

        PLEEPLOG_TRACE("Create Entities");
        // create entities
        // create component and pass or construct inline
        // if component is explicit (no initalizer list), we can omit template

        Entity frog = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(frog, TagComponent{ "froog" });
        m_currentCosmos->add_component(frog, TransformComponent(glm::vec3(1.5f, 2.0f, -0.5f)));
        // TODO: moment of inertia is not correct with scaled transform, scale has to be squared?
        m_currentCosmos->get_component<TransformComponent>(frog).scale = glm::vec3(0.2f, 0.2f, 0.2f);
        std::shared_ptr<Model> frogModel = std::make_shared<Model>("resources/normal_frog.obj");
        m_currentCosmos->add_component(frog, ModelComponent(frogModel));
        //m_currentCosmos->add_component(frog, PhysicsControlComponent{});
        PhysicsComponent frog_physics;
        frog_physics.mass = 30.0f;
        //frog_physics.angularVelocity = glm::vec3(0.2f, 0.0f, 0.2f);
        m_currentCosmos->add_component(frog, frog_physics);
        BoxColliderComponent frog_collider;
        frog_collider.m_localTransform.scale = glm::vec3(4.0f, 3.0f, 5.0f);
        frog_collider.m_localTransform.origin = glm::vec3(0.0f, 1.5f, 0.0f);
        m_currentCosmos->add_component(frog, frog_collider);
        m_currentCosmos->add_component(frog, RigidBodyComponent{});

/*
        Entity vamp = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(vamp, TransformComponent(glm::vec3(2.0f, 0.0f, 0.0f)));
        m_currentCosmos->get_component<TransformComponent>(vamp).scale = glm::vec3(0.01f, 0.01f, 0.01f);
        std::shared_ptr<Model> vampModel = std::make_shared<Model>("resources/vampire/dancing_vampire3.dae");
        m_currentCosmos->add_component(vamp, ModelComponent(vampModel));
*/

        Entity crate = m_currentCosmos->create_entity();
        TransformComponent crateTransform(glm::vec3(2.9f, 0.0f, 0.3f));
        m_currentCosmos->add_component(crate, crateTransform);
        m_currentCosmos->add_component(crate, ModelComponent(model_builder::create_cube("resources/container2.png", "resources/container2_specular.png")));
        PhysicsComponent crate_physics;
        //crate_physics.velocity = glm::vec3(-0.2f, 0.1f, 0.0f);
        crate_physics.angularVelocity = glm::vec3(0.0f, 0.7f, 0.2f);
        crate_physics.mass = 100.0f;
        m_currentCosmos->add_component(crate, crate_physics);
        m_currentCosmos->add_component(crate, BoxColliderComponent{});
        m_currentCosmos->add_component(crate, RigidBodyComponent{});

        Entity block = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(block, TransformComponent(glm::vec3(2.0f, -1.0f, 0.0f)));
        m_currentCosmos->get_component<TransformComponent>(block).orientation = glm::normalize(glm::angleAxis(glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
        m_currentCosmos->get_component<TransformComponent>(block).scale = glm::vec3(1.8f, 0.3f, 1.8f);
        m_currentCosmos->add_component(block, ModelComponent(model_builder::create_cube("resources/bricks2.jpg", "resources/bricks2_disp.jpg", "resources/bricks2_normal.jpg")));
        PhysicsComponent block_physics;
        //block_physics.velocity = glm::vec3(0.6f, 0.0f, -0.6f);
        //block_physics.angularVelocity = glm::vec3(0.2f, 0.0f, 0.3f);
        block_physics.lockOrigin = true;
        block_physics.lockedOrigin = m_currentCosmos->get_component<TransformComponent>(block).origin;
        //block_physics.lockOrientation = true;
        //block_physics.lockedOrientation = m_currentCosmos->get_component<TransformComponent>(block).orientation;
        block_physics.mass = 500.0f;
        m_currentCosmos->add_component(block, block_physics);
        m_currentCosmos->add_component(block, BoxColliderComponent{});
        m_currentCosmos->add_component(block, RigidBodyComponent{});

        Entity torus = m_currentCosmos->create_entity();
        TransformComponent torus_transform;
        torus_transform.origin = glm::vec3(0.0f, 1.0f, 0.0f);
        torus_transform.orientation = glm::angleAxis(glm::radians(10.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
        torus_transform.scale = glm::vec3(1.5f, 0.5f, 3.5f);
        m_currentCosmos->add_component(torus, torus_transform);
        std::shared_ptr<Model> torusModel = std::make_shared<Model>("resources/torus.obj");
        m_currentCosmos->add_component(torus, ModelComponent(model_builder::create_cube("resources/blending_transparent_window.png")));
        PhysicsComponent torus_physics;
        torus_physics.mass = 150.0f;
        m_currentCosmos->add_component(torus, torus_physics);
        BoxColliderComponent torus_collider;
        //torus_collider.m_localTransform.scale = glm::vec3(3.5f, 0.5f, 1.5f);
        torus_collider.m_responseType = CollisionResponseType::rigid;
        m_currentCosmos->add_component(torus, torus_collider);
        m_currentCosmos->add_component(torus, RigidBodyComponent{});

        Entity wall1 = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(wall1, TransformComponent(glm::vec3(1.5f, 0.5f, -1.5f)));
        m_currentCosmos->get_component<TransformComponent>(wall1).orientation = glm::angleAxis(1.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
        m_currentCosmos->add_component(wall1, ModelComponent(model_builder::create_quad("resources/wood.png", "resources/wood.png", "resources/toy_box_normal.png", "resources/toy_box_disp.png")));
        
        Entity wall2 = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(wall2, TransformComponent(glm::vec3(-1.0f, 1.0f, -1.0f)));
        m_currentCosmos->get_component<TransformComponent>(wall2).orientation = 
            glm::normalize(glm::angleAxis(1.0f, glm::vec3(0.0f, 1.0f, 0.0f)));
        m_currentCosmos->add_component(wall2, ModelComponent(model_builder::create_quad("resources/wood.png", "resources/wood.png", "resources/toy_box_normal.png", "resources/toy_box_disp.png")));

        Entity floor = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(floor, TransformComponent(glm::vec3(0.0f, -2.0f, 0.0f)));
        m_currentCosmos->get_component<TransformComponent>(floor).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        m_currentCosmos->get_component<TransformComponent>(floor).scale = glm::vec3(5.0f, 5.0f, 0.05f);
        m_currentCosmos->add_component(floor, ModelComponent(model_builder::create_quad("resources/brickwall.jpg", "resources/brickwall_specular.jpg", "resources/brickwall_normal_up.jpg")));//, "resources/spiral_disp.jpg")));
        PhysicsComponent floor_physics;
        // TODO: what mass to assign to non-dynamic objects? same as otherwise?
        // TODO: in general generate mass from known density
        floor_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        floor_physics.lockOrigin = true;
        floor_physics.lockedOrigin = m_currentCosmos->get_component<TransformComponent>(floor).origin;
        floor_physics.lockOrientation = true;
        floor_physics.lockedOrientation = m_currentCosmos->get_component<TransformComponent>(floor).orientation;
        m_currentCosmos->add_component(floor, floor_physics);
        m_currentCosmos->add_component(floor, BoxColliderComponent{});
        m_currentCosmos->get_component<BoxColliderComponent>(floor).m_localTransform.origin.z = -0.499f;
        m_currentCosmos->add_component(floor, RigidBodyComponent{});

        Entity snow = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(snow, TransformComponent(glm::vec3(5.01f, -2.0f, 0.0f)));
        m_currentCosmos->get_component<TransformComponent>(snow).orientation = 
            glm::normalize(glm::angleAxis(glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
        m_currentCosmos->get_component<TransformComponent>(snow).scale = glm::vec3(5.0f, 5.0f, 0.05f);
        m_currentCosmos->add_component(snow, ModelComponent(model_builder::create_quad("resources/snow-packed12-Base_Color.png", "resources/snow-packed12-Specular.png", "resources/snow-packed12-normal-ogl.png", "resources/snow-packed12-Height.png")));
        PhysicsComponent snow_physics;
        // TODO: what mass to assign to non-dynamic objects?
        // TODO: in general generate mass from known density
        snow_physics.mass = INFINITE_MASS;//5.0f * 500.0f;
        snow_physics.lockOrigin = true;
        snow_physics.lockedOrigin = m_currentCosmos->get_component<TransformComponent>(snow).origin;
        snow_physics.lockOrientation = true;
        snow_physics.lockedOrientation = m_currentCosmos->get_component<TransformComponent>(snow).orientation;
        m_currentCosmos->add_component(snow, snow_physics);
        m_currentCosmos->add_component(snow, BoxColliderComponent{});
        m_currentCosmos->get_component<BoxColliderComponent>(snow).m_localTransform.origin.z = -0.5f;
        m_currentCosmos->add_component(snow, RigidBodyComponent{});

        // Scene needs to create an entity with camera component
        Entity mainCamera = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(mainCamera, TransformComponent(glm::vec3(0.0f, 2.5f, 6.0f)));
        PhysicsComponent mainCamera_physics;
        mainCamera_physics.isAsleep = true;
        m_currentCosmos->add_component(mainCamera, mainCamera_physics);
        m_currentCosmos->get_component<TransformComponent>(mainCamera).orientation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, -0.2f));
        m_currentCosmos->add_component(mainCamera, CameraComponent());
        CameraControlComponent mainCamera_control;
        mainCamera_control.m_target = torus;
        m_currentCosmos->add_component(mainCamera, mainCamera_control);
        
        // then it needs to be assigned somewhere in render pipeline (view camera, shadow camera, etc)
        // assuming there is only ever 1 main camera we can notify over event broker
        // dynamos don't have acess to cosmos, so they can't lookup entity
        // synchro can maintain camera and pass its data each frame
        
        Event cameraEvent(events::rendering::SET_MAIN_CAMERA);
        events::rendering::SET_MAIN_CAMERA_params cameraParams {
            mainCamera
        };
        cameraEvent.set_param(cameraParams);
        
        // TODO: unit testing
        renderSynchro->attach_dynamo(nullptr);
        m_eventBroker->send_event(cameraEvent);
        renderSynchro->attach_dynamo(m_renderDynamo);

        m_eventBroker->send_event(cameraEvent);


        Entity light = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(light, TransformComponent(glm::vec3(0.0f, 1.0f, 0.0f)));
        m_currentCosmos->get_component<TransformComponent>(light).scale = glm::vec3(0.1f);
        // remember this is relative to exposure
        m_currentCosmos->add_component(light, LightSourceComponent(glm::vec3(4.0f, 4.0f, 4.0f)));
        m_currentCosmos->add_component(light, ModelComponent(model_builder::create_cube("resources/blending_transparent_window.png")));

        // RenderRelays should deal with each render phase
        //   and need to know camera entity's data
        // RenderRelays don't have access to Cosmos, so they need references to camera data
        // Cosmos builder could create relays for RenderDynamo, create entities and then assign
        //  how would it deal with dynamically created cameras...
        // Camera entities, like "rendered" entities need to have/know a relay type
        //   then the camera synchro can pass to dynamo and it would know what to do?
    }
    
    void CosmosContext::_quit_handler(Event quitEvent)
    {
        // should only be subscribed to events given with type:
        // events::window::QUIT
        UNREFERENCED_PARAMETER(quitEvent);
        PLEEPLOG_TRACE("Handling event " + std::to_string(events::window::QUIT) + " (events::window::QUIT)");

        this->stop();
    }

}