add_library(GLM_MODULE STATIC src/empty.cpp)

add_subdirectory(ext)

target_link_libraries(GLM_MODULE PUBLIC glm)
target_include_directories(GLM_MODULE PUBLIC "${CMAKE_CURRENT_LIST_DIR}/ext")
