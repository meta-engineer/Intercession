# Interally built self-contained libraries

# library enabling/options
option(USE_VIEW_MANAGER "Library for managing graphic view" OFF)

# library integration
if(USE_VIEW_MANAGER)
    add_subdirectory(view_manager)
    target_link_libraries(${PROJECT_NAME} view_manager)
    target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/source/libraries/view_manager)
endif()
