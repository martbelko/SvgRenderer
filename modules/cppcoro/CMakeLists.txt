add_library(CPPCORO_MODULE STATIC src/empty.cpp)

add_subdirectory(ext)

target_link_libraries(CPPCORO_MODULE PUBLIC cppcoro)
target_include_directories(CPPCORO_MODULE PUBLIC "${CMAKE_CURRENT_LIST_DIR}/ext/include")
