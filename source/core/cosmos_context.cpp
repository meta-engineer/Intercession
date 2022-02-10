#include "cosmos_context.h"

#include "logging/pleep_log.h"

#include "controlling/control_synchro.h"
#include "rendering/render_synchro.h"

#include "physics/transform_component.h"

// TODO: This is temporary for hard-coding entities
#include "rendering/model_component.h"
#include "rendering/cube_model.h"

namespace pleep
{
    CosmosContext::CosmosContext()
    {
        m_running = false;

        // build event listener and setup context's "fallback" listeners
        m_eventBroker = new EventBroker();
        m_eventBroker->add_listener(METHOD_LISTENER(events::window::QUIT, CosmosContext::_quit_handler));

        // enforce dynamos are built?
        m_renderDynamo = nullptr;
        m_controlDynamo = nullptr;
        m_currentCosmos = nullptr;
    }
    
    CosmosContext::CosmosContext(GLFWwindow* windowApi)
        : CosmosContext()
    {
        // Broker is ready to be distributed

        // construct dynamos
        m_renderDynamo = new RenderDynamo(m_eventBroker, windowApi);
        m_controlDynamo  = new ControlDynamo(m_eventBroker, windowApi);
        
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
        m_currentCosmos->register_component<ModelComponent>();

        // register/create synchros, set component signatures
        // we shouldn't need to keep synchro references after we config them here, 
        // we'll only access through Cosmos
        // any other functionallity should be in dynamos
        std::shared_ptr<ControlSynchro> controlSynchro = m_currentCosmos->register_synchro<ControlSynchro>();
        controlSynchro->attach_dynamo(m_controlDynamo);
        {
            Signature sign;
            sign.set(m_currentCosmos->get_component_type<TransformComponent>());

            m_currentCosmos->set_synchro_signature<ControlSynchro>(sign);
        }

        std::shared_ptr<RenderSynchro> renderSynchro   = m_currentCosmos->register_synchro<RenderSynchro>();
        renderSynchro->attach_dynamo(m_renderDynamo);
        {
            Signature sign;
            sign.set(m_currentCosmos->get_component_type<TransformComponent>());

            m_currentCosmos->set_synchro_signature<RenderSynchro>(sign);
        }

        // create entities
        // create component and pass or construct inline
        // if component is explicit (no initalizer list), we can omit template
        Entity cube = m_currentCosmos->create_entity();
        m_currentCosmos->add_component(cube, TransformComponent(glm::vec3(2.0f)));
        std::shared_ptr<Model> cubeModel = std::make_shared<Model>("resources/normal_frog.obj");
        m_currentCosmos->add_component(cube, ModelComponent(cubeModel));

        Entity box  = m_currentCosmos->create_entity();
        TransformComponent boxTransform(glm::vec3(1.0f));
        m_currentCosmos->add_component(box, boxTransform);
        m_currentCosmos->add_component(box, ModelComponent(create_cube_model_ptr("resources/container2.png", "resources/container2_specular.png")));

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