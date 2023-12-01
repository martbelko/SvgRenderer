add_library(XML_MODULE STATIC src/empty.cpp)

add_subdirectory(ext)

target_link_libraries(XML_MODULE PUBLIC tinyxml2)
target_include_directories(XML_MODULE PUBLIC "${CMAKE_CURRENT_LIST_DIR}/ext")
