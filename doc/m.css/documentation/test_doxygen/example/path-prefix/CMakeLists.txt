configure_file(${CMAKE_CURRENT_SOURCE_DIR}/configure.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/configure.h)

add_executable(app main.cpp)
target_include_directories(app ${CMAKE_CURRENT_BINARY_DIR})
