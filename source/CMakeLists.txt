# Use source/source_files.cmake to configure compilation lists for each target

# PROJECT_SOURCE_DIR is cmake source(root CMakeLists)
target_include_directories(${ENGINE_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/source)
target_include_directories(${SERVER_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/source)
target_include_directories(${CLIENT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/source)

# link internal libraries to executable ${PROJECT_NAME}
add_subdirectory(libraries)
