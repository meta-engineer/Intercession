
# PROJECT_SOURCE_DIR is cmake source(root CMakeLists)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/source)

# link internal libraries to executable ${PROJECT_NAME}
add_subdirectory(libraries)

# include base libraries folder, #includes should be explicit to each library
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/source/libraries)