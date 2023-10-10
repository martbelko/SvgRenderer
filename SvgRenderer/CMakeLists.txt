project("SvgRenderer")

add_executable(${PROJECT_NAME} "SvgRenderer.cpp" "SvgRenderer.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()
