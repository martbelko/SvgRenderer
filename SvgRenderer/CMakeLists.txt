project("SvgRenderer" CXX)

add_executable(${PROJECT_NAME} "SvgRenderer.cpp" "SvgRenderer.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC GLFW_MODULE GLAD_MODULE)

add_compile_definitions(GLFW_INCLUDE_NONE)
