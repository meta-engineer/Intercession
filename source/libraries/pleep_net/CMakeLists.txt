add_library(pleep_net pleep_net.cpp)

# pleep_net links out to external asio (header only)
target_include_directories(pleep_net PUBLIC ${PROJECT_SOURCE_DIR}/external/asio/include)

# also include pleeplog
target_include_directories(pleep_net PUBLIC ${PROJECT_SOURCE_DIR}/logging)

# include pleep_net library in installer
install(TARGETS pleep_net   DESTINATION lib)
install(FILES   pleep_net.h DESTINATION include)
