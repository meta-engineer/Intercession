
set(COMMON_SOURCE_FILES
    source/core/i_app_gateway.cpp
    source/core/i_cosmos_context.cpp
    source/core/cosmos.cpp

    source/logging/pleep_logger.cpp

    source/events/event_broker.cpp

    source/rendering/render_dynamo.cpp
    source/rendering/render_synchro.cpp
    source/rendering/lighting_synchro.cpp
    source/rendering/model.cpp
    source/rendering/model_builder.cpp
    source/rendering/mesh.cpp
    source/rendering/shader_manager.cpp

    source/inputting/input_dynamo.cpp
    source/inputting/spacial_input_synchro.cpp

    source/physics/physics_dynamo.cpp
    source/physics/physics_synchro.cpp
    source/physics/box_collider_synchro.cpp
    source/physics/ray_collider_synchro.cpp
    source/physics/box_collider_component.cpp
    source/physics/ray_collider_component.cpp
    source/physics/rigid_body_component.cpp
    source/physics/spring_body_component.cpp

    source/networking/network_synchro.cpp

    source/scripting/i_script_drivetrain.cpp
    source/scripting/script_dynamo.cpp
    source/scripting/script_synchro.cpp
    source/scripting/script_library.cpp
)

set(CLIENT_SOURCE_FILES
    source/client_main.cpp
    source/client/client_app_gateway.cpp
    source/client/client_cosmos_context.cpp

    source/client/client_network_dynamo.cpp
)

set(SERVER_SOURCE_FILES
    source/server_main.cpp
    source/server/server_app_gateway.cpp
    source/server/server_cosmos_context.cpp

    source/server/server_network_dynamo.cpp
)