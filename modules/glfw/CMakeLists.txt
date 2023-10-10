add_library(GLFW_MODULE STATIC src/empty.cpp)

add_subdirectory(ext)

target_link_libraries(GLFW_MODULE PUBLIC glfw)
target_include_directories(GLFW_MODULE PUBLIC "${CMAKE_CURRENT_LIST_DIR}/ext/include")
