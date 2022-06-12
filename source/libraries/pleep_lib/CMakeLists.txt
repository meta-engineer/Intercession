add_library(pleep_lib pleep_lib.cpp)

# link to external resources (does it make sense for a library to have OTHER library dependancies?)
#target_include_directories(pleep_lib ${PROJECT_SOURCE_DIR}/external/something)

# include pleep_net library in installer
install(TARGETS pleep_lib   DESTINATION lib)
install(FILES   pleep_lib.h DESTINATION include)
