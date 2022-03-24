#include "cosmos_context.h"

#include "logging/pleep_log.h"

// TODO: This is temporary for building hard-coded entities
#include "controlling/control_synchro.h"
#include "rendering/render_synchro.h"
#include "rendering/lighting_synchro.h"
#include "physics/physics_synchro.h"

#include "physics/transform_component.h"
#include "physics/physics_component.h"
#include "physics/box_collider.h"
#include "controlling/control_component.h"
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
            m_currentCosmos->update(deltaTime);

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
        m_currentCosmos->register_component<ControlComponent>();
        m_currentCosmos->register_component<ModelComponent>();
        m_currentCosmos->register_component<CameraComponent>();
        m_currentCosmos->register_component<LightSourceComponent>();
        m_currentCosmos->register_component<PhysicsComponent>();
        // register tag component as a normal component
        m_currentCosmos->register_component<TagComponent>();

        // register/create synchros, set component signatures
        // we shouldn't need to keep synchro references after we config them here, 
        // we'll only access through Cosmos
        // any other functionallity should be in dynamos
        PLEEPLOG_TRACE("Create Synchros");
        std::shared_ptr<ControlSynchro> controlSynchro = m_currentCosmos->register_synchro<ControlSynchro>();
        controlSynchro->attach_dynamo(m_controlDynamo);
        {
            Signature sign;
            sign.set(m_currentCosmos->get_component_type<ControlComponent>());

            m_currentCosmos->set_synchro_signature<ControlSynchro>(sign);
        }

        // synchros are in a map so it isn't guarenteed that LightingSynchro is invoked before RenderSynchro
        std::shared_ptr<LightingSynchro> lightingSynchro = m_currentCosmos->register_synchro<LightingSynchro>();
        lightingSynchro->attach_dynamo(m_renderDynamo);
        {
            Signature sign;
            sign.set(m_currentCosmos->get_component_type<TransformComponent>());
            sign.set(m_currentCosmos->get_component_type<LightSourceComponent>());

            m_currentCosmos->set_synchro_signature<LightingSynchro>(sign);
        }

        std::shared_ptr<RenderSynchro> renderSynchro = m_currentCosmos->register_synchro<RenderSynchro>();
        renderSynchro->attach_dynamo(m_renderDynamo);
        {
            Signature sign;
            sign.set(m_currentCosmos->get_component_type<TransformComponent>());
            sign.set(m_currentCosmos->get_component_type<ModelComponent>());

            m_currentCosmos->set_synchro_signature<RenderSynchro>(sign);
        }

        std::shared_ptr<PhysicsSynchro> physicsSynchro = m_currentCosmos->register_synchro<PhysicsSynchro>();
        physicsSynchro->attach_dynamo(m_physicsDynamo);
        {
            Signature sign;
            sign.set(m_currentCosmos->get_component_type<TransformComponent>());
            sign.set(m_currentCosmos->get_component_type<PhysicsComponent>());

            m_currentCosmos->set_synchro_signature<PhysicsSynchro>(sign);
        }

        PLEEPLOG_TRACE("Create Entities");
        // create entities
        // create component and pass or construct inline
        // if component is explicit (no initalizer list), we can omit template
        Entity frog = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(frog, TagComponent{ "froog" });
        m_currentCosmos->add_component(frog, TransformComponent(glm::vec3(0.1f, 0.0f, -2.0f)));
        m_currentCosmos->get_component<TransformComponent>(frog).scale = glm::vec3(0.2f, 0.2f, 0.2f);
        std::shared_ptr<Model> frogModel = std::make_shared<Model>("resources/normal_frog.obj");
        m_currentCosmos->add_component(frog, ModelComponent(frogModel));
        m_currentCosmos->add_component(frog, PhysicsComponent{});
        PhysicsComponent& frog_physics = m_currentCosmos->get_component<PhysicsComponent>(frog);
        frog_physics.angularVelocity = glm::vec3(0.0f, 1.0f, 0.0f);
        frog_physics.collider = std::make_shared<BoxCollider>();

        Entity vamp = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(vamp, TransformComponent(glm::vec3(2.0f, 0.0f, 0.0f)));
        m_currentCosmos->get_component<TransformComponent>(vamp).scale = glm::vec3(0.01f, 0.01f, 0.01f);
        std::shared_ptr<Model> vampModel = std::make_shared<Model>("resources/vampire/dancing_vampire3.dae");
        m_currentCosmos->add_component(vamp, ModelComponent(vampModel));

        Entity crate = m_currentCosmos->create_entity();
        TransformComponent crateTransform(glm::vec3(1.0f, -1.0f, 1.0f));
        m_currentCosmos->add_component(crate, crateTransform);
        m_currentCosmos->add_component(crate, ModelComponent(model_builder::create_cube("resources/container2.png", "resources/container2_specular.png")));
        m_currentCosmos->add_component(crate, PhysicsComponent{});
        PhysicsComponent& crate_physics = m_currentCosmos->get_component<PhysicsComponent>(crate);
        crate_physics.velocity = glm::vec3(-0.2f, 0.1f, 0.0f);
        crate_physics.angularVelocity = glm::vec3(0.0f, 0.1f, 0.0f);
        crate_physics.collider = std::make_shared<BoxCollider>();

        Entity block = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(block, TransformComponent(glm::vec3(-1.0f, -0.6f, 0.0f)));
        m_currentCosmos->add_component(block, ModelComponent(model_builder::create_cube("resources/bricks2.jpg", "resources/bricks2_disp.jpg", "resources/bricks2_normal.jpg")));
        m_currentCosmos->add_component(block, PhysicsComponent{});
        PhysicsComponent& block_physics = m_currentCosmos->get_component<PhysicsComponent>(block);
        block_physics.velocity = glm::vec3(0.1f, 0.0f, 0.2f);
        block_physics.angularVelocity = glm::vec3(0.0f, 0.0f, 0.1f);
        block_physics.collider = std::make_shared<BoxCollider>();

        Entity wall1 = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(wall1, TransformComponent(glm::vec3(1.5f, 0.5f, -1.5f)));
        m_currentCosmos->get_component<TransformComponent>(wall1).rotation.x += 1.0f;
        m_currentCosmos->add_component(wall1, ModelComponent(model_builder::create_quad("resources/wood.png", "resources/wood.png", "resources/toy_box_normal.png", "resources/toy_box_disp.png")));
        
        Entity wall2 = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(wall2, TransformComponent(glm::vec3(-1.0f, 1.0f, -1.0f)));
        m_currentCosmos->get_component<TransformComponent>(wall2).rotation.y += 1.0f;
        m_currentCosmos->add_component(wall2, ModelComponent(model_builder::create_quad("resources/wood.png", "resources/wood.png", "resources/toy_box_normal.png", "resources/toy_box_disp.png")));

        Entity floor = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(floor, TransformComponent(glm::vec3(0.0f, -2.0f, 0.0f)));
        m_currentCosmos->get_component<TransformComponent>(floor).rotation.x += glm::radians(90.0f);
        m_currentCosmos->get_component<TransformComponent>(floor).scale = glm::vec3(5.0f, 5.0f, 0.05f);
        m_currentCosmos->add_component(floor, ModelComponent(model_builder::create_quad("resources/brickwall.jpg", "resources/brickwall_specular.jpg", "resources/brickwall_normal_up.jpg")));//, "resources/spiral_disp.jpg")));
        m_currentCosmos->add_component(floor, PhysicsComponent{});
        PhysicsComponent& floor_physics = m_currentCosmos->get_component<PhysicsComponent>(floor);
        floor_physics.collider = std::make_shared<BoxCollider>();
        floor_physics.isDynamic = false;

        Entity torus = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(torus, TransformComponent(glm::vec3(-1.0f)));
        std::shared_ptr<Model> torusModel = std::make_shared<Model>("resources/torus.obj");
        m_currentCosmos->add_component(torus, ModelComponent(torusModel));

        // Scene needs to create an entity with camera component
        Entity mainCamera = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(mainCamera, TransformComponent(glm::vec3(0.0f, 2.5f, 6.0f)));
        m_currentCosmos->get_component<TransformComponent>(mainCamera).rotation = glm::vec3(glm::radians(-30.0f), glm::radians(180.0f), 0.0f);
        m_currentCosmos->add_component(mainCamera, CameraComponent());
        m_currentCosmos->add_component(mainCamera, ControlComponent{});
        
        // then it needs to be assigned somewhere in render pipeline (view camera, shadow camera, etc)
        // assuming there is only ever 1 main camera we can notify over event broker
        // dynamos don't have acess to cosmos, so they can't lookup entity
        // synchro can maintain camera and pass its data each frame
        
        Event cameraEvent(events::rendering::SET_MAIN_CAMERA);
        events::rendering::set_main_camera::Params cameraParams {
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