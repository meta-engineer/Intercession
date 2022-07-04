# Interally built self-contained libraries

# include base libraries folder, #includes should be explicit to each library
target_include_directories(${ENGINE_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/source/libraries)

# I don't know when it makes sense to create a library instead of integrating
# directly into the user source. If the source is directly dependant on the "library"
# symbols, is it at all helpful to separate into a contained library?

# I think making a library is justified if the content is self-contained on both sides:
# therefore it doesn't rely on linking to another library 
# and it isn't tightly coupled to the user application

# library enabling/options
# This only removes library linkage, references to library components
# in the user codebase must also be preprocessed out by cmakedefine directive
# (options are only available to user codebase, not library)
option(USE_PLEEP_LIB "Example to test library linking" OFF)
if (USE_PLEEP_LIB)
    add_subdirectory(pleep_lib)
    target_link_libraries(${ENGINE_NAME} pleep_lib)
endif()
