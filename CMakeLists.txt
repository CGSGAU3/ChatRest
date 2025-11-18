cmake_minimum_required( VERSION 3.22 FATAL_ERROR )

project(server LANGUAGES CXX)

include( ${PROJECT_SOURCE_DIR}/cmake/cpm.cmake )
include( ${PROJECT_SOURCE_DIR}/src/src.cmake )
include( ${PROJECT_SOURCE_DIR}/cmake/external.cmake )
include( ${PROJECT_SOURCE_DIR}/tests/tests.cmake )

add_executable( ${PROJECT_NAME} )

target_sources( ${PROJECT_NAME}
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${SERVER_SOURCES}
)

target_include_directories( ${PROJECT_NAME}
    PRIVATE
    ${SERVER_INCLUDES}
)

target_compile_features( ${PROJECT_NAME}
    PRIVATE
    cxx_std_20
)

target_link_libraries( ${PROJECT_NAME}
    PRIVATE
    ${SERVER_LIBS}
)
