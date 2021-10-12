add_library(view_manager view_manager.cpp)

# view manager links out to external/glfw (not ideal dependancies)
target_link_libraries(view_manager glfw)
target_include_directories(view_manager PUBLIC ${PROJECT_SOURCE_DIR}/external/glfw/include)

# include library in installer
install(TARGETS view_manager   DESTINATION lib)
install(FILES   view_manager.h DESTINATION include)