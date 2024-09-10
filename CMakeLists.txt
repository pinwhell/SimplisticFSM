cmake_minimum_required(VERSION 3.16)
project(sfsm)
add_library(sfsm INTERFACE)
target_include_directories(sfsm INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
add_subdirectory(examples)

install(TARGETS sfsm
        EXPORT sfsm-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)

install(EXPORT sfsm-targets
        FILE sfsm-targets.cmake
        NAMESPACE sfsm::
        DESTINATION lib/cmake/sfsm)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include)

configure_file(cmake/sfsm-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/sfsm-config.cmake
    @ONLY)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sfsm-config.cmake
    DESTINATION lib/cmake/sfsm)