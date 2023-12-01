project("SvgRenderer" CXX)

file(GLOB_RECURSE project_headers ${CMAKE_CURRENT_LIST_DIR}/*.h)
file(GLOB_RECURSE project_sources ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${project_headers} ${project_sources})

target_precompile_headers(${PROJECT_NAME} PRIVATE srpch.h)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC GLFW_MODULE GLAD_MODULE GLM_MODULE XML_MODULE CPPCORO_MODULE)

add_compile_definitions(GLFW_INCLUDE_NONE)

include_directories(./)
