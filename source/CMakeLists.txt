
# link internal libraries to executable ${PROJECT_NAME}
option(USE_VIEW_MANAGER "Library for managing graphic view" ON)

# TODO: set variable for all libraries to add at once

if(USE_VIEW_MANAGER)
    add_subdirectory(view_manager)
    target_link_libraries(${PROJECT_NAME} view_manager)
endif()