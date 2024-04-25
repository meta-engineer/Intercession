
set(COMMON_SOURCE_FILES
    source/core/i_app_gateway.cpp
    source/core/i_cosmos_context.cpp
    source/core/cosmos.cpp

    source/staging/cosmos_builder.cpp
    source/staging/iceberg_cosmos.cpp
    source/staging/moon_cosmos.cpp

    source/logging/pleep_logger.cpp

    source/events/event_broker.cpp

    source/rendering/render_dynamo.cpp
    source/rendering/render_synchro.cpp
    source/rendering/lighting_synchro.cpp
    source/rendering/mesh.cpp
    source/rendering/texture.cpp
    source/rendering/shader_manager.cpp
    source/rendering/model_manager.cpp
    source/rendering/model_manager_faux.cpp

    source/inputting/input_dynamo.cpp
    source/inputting/spacial_input_synchro.cpp

    source/physics/physics_dynamo.cpp
    source/physics/physics_synchro.cpp
    source/physics/collider_synchro.cpp
    source/physics/collision_procedures.cpp

    source/networking/network_synchro.cpp
    source/networking/timeline_api.cpp

    source/spacetime/parallel_cosmos_context.cpp
    source/spacetime/parallel_network_dynamo.cpp

    source/behaviors/i_behaviors_drivetrain.cpp
    source/behaviors/behaviors_dynamo.cpp
    source/behaviors/behaviors_synchro.cpp
    source/behaviors/behaviors_library.cpp
)

set(CLIENT_SOURCE_FILES
    source/client_main.cpp
    source/client/client_app_gateway.cpp
    source/client/client_cosmos_context.cpp

    source/client/client_model_cache.cpp

    source/client/client_network_dynamo.cpp
)

set(SERVER_SOURCE_FILES
    source/server_main.cpp
    source/server/server_app_gateway.cpp
    source/server/server_cosmos_context.cpp
    
    source/server/server_model_cache.cpp

    source/server/server_network_dynamo.cpp
)