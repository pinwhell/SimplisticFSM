cmake_minimum_required(VERSION 3.16)
project(simplistic-fsm)
add_library(fsm INTERFACE)
add_library(simplistic::fsm ALIAS fsm)
target_include_directories(fsm INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
add_subdirectory(examples)

install(TARGETS fsm
        EXPORT simplistic-fsm-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)

install(EXPORT simplistic-fsm-targets
        FILE simplistic-fsm-targets.cmake
        NAMESPACE simplistic::
        DESTINATION lib/cmake/simplistic)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include)

configure_file(cmake/simplistic-fsm-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/simplistic-fsm-config.cmake
    @ONLY)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/simplistic-fsm-config.cmake
    DESTINATION lib/cmake/simplistic)