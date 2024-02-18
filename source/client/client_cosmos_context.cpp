#include "client_cosmos_context.h"

// TODO: This is temporary until proper cosmos staging is implemented
#include "staging/test_cosmos.h"
#include "staging/client_focal_entity.h"
#include "staging/client_local_entities.h"
#include "staging/hard_config_cosmos.h"

namespace pleep
{
    ClientCosmosContext::ClientCosmosContext(GLFWwindow* windowApi)
        : I_CosmosContext()
    {
        // I_CosmosContext() has setup broker
        
        // construct dynamos
        m_dynamoCluster.inputter  = std::make_shared<InputDynamo>(m_eventBroker, windowApi);
        m_dynamoCluster.networker = std::make_shared<ClientNetworkDynamo>(m_eventBroker);
        m_dynamoCluster.behaver   = std::make_shared<BehaviorsDynamo>(m_eventBroker);
        m_dynamoCluster.physicser = std::make_shared<PhysicsDynamo>(m_eventBroker);
        m_dynamoCluster.renderer  = std::make_shared<RenderDynamo>(m_eventBroker, windowApi);

        // build and populate starting cosmos
        _build_cosmos();
        // TODO: use network dynamo to get cosmos config from server (some loading cosmos while waiting?)

        // TODO: call network dynamo to setup connection
        // How does this method fit into the interface?
        //m_dynamoCluster.networker->setup_connection();
    }
    
    ClientCosmosContext::~ClientCosmosContext() 
    {
        // delete cosmos first to avoid null dynamo dereferences?
        // TODO: deleting entities could cause dynamos to have null references... exit needs to be more secure
        // investigate weak pointers? dynamos can check if pointer is still valid before use
        m_currentCosmos = nullptr;
    }
    
    void ClientCosmosContext::_prime_frame() 
    {
        // ***** Cosmos Update *****
        // invokes all registered synchros to process their entities
        // this fills the dynamo relays with packets which should be cleared in _clean_frame()
        if (m_currentCosmos) m_currentCosmos->update();
    }
    
    void ClientCosmosContext::_on_fixed(double fixedTime)
    {
        // TODO: give each dynamo a run "fixed" & variable method so we don't need to explicitly
        //   know which dynamos to call fixed and which to call on frametime
        m_dynamoCluster.inputter->run_relays(fixedTime);
        m_dynamoCluster.networker->run_relays(fixedTime);
        m_dynamoCluster.behaver->run_relays(fixedTime);
        m_dynamoCluster.physicser->run_relays(fixedTime);
    }
    
    void ClientCosmosContext::_on_frame(double deltaTime) 
    {
        m_dynamoCluster.renderer->run_relays(deltaTime);

        // ***** Post Processing *****
        // top ui layer in context for debug
        // TODO: abstract this to ui layer
        // TODO: implment some fetchable stats in Dynamos to show here
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // show ui window
        {
            ImGuiWindowFlags overlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
            static bool p_open;
            
            static int location = 0;
            const float PAD = 10.0f;
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
            ImVec2 work_size = viewport->WorkSize;
            ImVec2 window_pos, window_pos_pivot;
            window_pos.x = (location & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
            window_pos.y = (location & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
            window_pos_pivot.x = (location & 1) ? 1.0f : 0.0f;
            window_pos_pivot.y = (location & 2) ? 1.0f : 0.0f;
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

            // Create a window and append into it.
            ImGui::Begin("Client Context Debug", &p_open, overlayFlags);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            // Report Cosmos info
            if (m_currentCosmos)
            {
                std::string entityCountString = " Total Entity Count: " + std::to_string(m_currentCosmos->get_entity_count());
                ImGui::Text(entityCountString.c_str());

                std::string hostedCountString = "Hosted Entity Count: " + std::to_string(m_currentCosmos->get_num_hosted_entities());
                ImGui::Text(hostedCountString.c_str());

                std::string focalEntityString = "Focal entity: " + std::to_string(m_currentCosmos->get_focal_entity());
                ImGui::Text(focalEntityString.c_str());

                ImGui::Text(("Update count: " + std::to_string(m_currentCosmos->get_coherency())).c_str());
            }
            
            // Display some text (you can use a format strings too)
            //ImGui::Text("This window runs outside of the cosmos");

            // Edit bools storing our window open/close state
            //static bool checkbox;
            //ImGui::Checkbox("checkbox", &checkbox);

            // Edit 1 float using a slider from 0.0f to 1.0f
            //static float f = 0.0f;
            //ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            // Edit 3 floats representing a color
            //ImGui::ColorEdit3("clear color", (float*)&clear_color);

            static uint16_t nextTimesliceId = 0;
            static int currTimesliceId = 0;

            ImGui::Text("NetworkDynamo%sconnected",
                 m_dynamoCluster.networker->get_num_connections() < 1 ? " is NOT " : " IS ");

            /// TODO: how to determine what timeslice we are connected to since client has id NULL_TIMESLICEID for logistics purposes?
            static float timesliceData[TIMESLICEID_SIZE]={0.0f};
            static events::network::APP_INFO_params appInfo{};
            timesliceData[appInfo.currentTimeslice] = 0.0f;
            // get new timeslice
            appInfo = m_dynamoCluster.networker->get_app_info();
            timesliceData[appInfo.currentTimeslice] = 1.0f;
            
            ImGui::Text("                    Present <--------------- Past");
            ImGui::Text("Current Timeslice:");
            ImGui::SameLine();
            ImGui::PlotHistogram("", timesliceData, static_cast<int>(appInfo.totalTimeslices), 0, std::to_string(appInfo.currentTimeslice).c_str());

            // Buttons return true when clicked (most widgets return true when edited/activated)
            if (ImGui::Button("Reconnect"))
            {
                // need to clear cosmos
                EventMessage clearMsg(events::cosmos::CONDEMN_ALL);
                m_eventBroker->send_event(clearMsg);
                // and call network to disconnect and reconnect
                // TODO: build address/port from config
                m_dynamoCluster.networker->restart_connection("127.0.0.1", 61336 + nextTimesliceId);
            }
            ImGui::SameLine();
            ImGui::Text("to timeslice %d", nextTimesliceId);
            ImGui::SameLine();
            if (ImGui::Button("+") && nextTimesliceId+1 < appInfo.totalTimeslices) nextTimesliceId++;
            ImGui::SameLine();
            if (ImGui::Button("-") && nextTimesliceId > 0) nextTimesliceId--;
            

            ImGui::End();
        }
        // Render out ui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    
    void ClientCosmosContext::_clean_frame() 
    {
        // TODO: Maybe A_Dynamo should have a generic "flush" method
        //   and we invoke all dynamos (to avoid having to specify)
        
        // flush dynamos of all synchro submissions
        m_dynamoCluster.inputter->reset_relays();
        m_dynamoCluster.networker->reset_relays();
        m_dynamoCluster.behaver->reset_relays();
        m_dynamoCluster.physicser->reset_relays();
        m_dynamoCluster.renderer->reset_relays();   // render dynamo will flush framebuffer
    }

    
    void ClientCosmosContext::_build_cosmos()
    {
        PLEEPLOG_TRACE("Start cosmos construction");

        // we need to build synchros and link them with dynamos
        // until we can load from server we can manually call methods to build entities in its ecs
        m_currentCosmos = construct_hard_config_cosmos(m_eventBroker, m_dynamoCluster);
/* 
        m_currentCosmos = build_test_cosmos(m_eventBroker, m_dynamoCluster);
        m_currentCosmos->set_focal_entity(
            create_client_focal_entity(m_currentCosmos, m_eventBroker)
        );
        create_client_local_entities(m_currentCosmos, m_eventBroker);
 */
        // use imgui input in main loop do add more at runtime?
        
        PLEEPLOG_TRACE("Done cosmos construction");
    }
}