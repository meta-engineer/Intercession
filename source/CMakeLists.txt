
# non library code
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/source)

# TODO: set variable for all libraries to add at once

# link internal libraries to executable ${PROJECT_NAME}
option(USE_VIEW_MANAGER "Library for managing graphic view" OFF)
if(USE_VIEW_MANAGER)
    add_subdirectory(view_manager)
    target_link_libraries(${PROJECT_NAME} view_manager)
endif()
