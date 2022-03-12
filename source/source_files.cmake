
set(ALL_SOURCE_FILES
    source/logging/pleep_logger.cpp

    source/main.cpp
    source/core/app_gateway.cpp

    source/core/cosmos_context.cpp
    source/core/cosmos.cpp
    source/core/i_dynamo.cpp

    source/events/event_broker.cpp

    source/rendering/render_dynamo.cpp
    source/rendering/render_synchro.cpp
    source/rendering/lighting_synchro.cpp
    source/rendering/model.cpp
    source/rendering/model_builder.cpp
    source/rendering/mesh.cpp
    source/rendering/shader_manager.cpp

    source/controlling/control_dynamo.cpp
    source/controlling/control_synchro.cpp

    source/physics/physics_dynamo.cpp
    source/physics/physics_synchro.cpp
)