file(GLOB_RECURSE module_headers ${CMAKE_CURRENT_LIST_DIR}/ext/include/*.h)
file(GLOB_RECURSE module_sources ${CMAKE_CURRENT_LIST_DIR}/ext/src/*.c)

add_library(GLAD_MODULE STATIC ${module_sources} ${module_headers})

target_include_directories(GLAD_MODULE PUBLIC "${CMAKE_CURRENT_LIST_DIR}/ext/include")
