# Interally built self-contained libraries

# library enabling/options
option(USE_PLEEP_NET "Library for abstracting asio sockets" ON)
if (USE_PLEEP_NET)
    add_subdirectory(pleep_net)
    target_link_libraries(${PROJECT_NAME} pleep_net)
endif()