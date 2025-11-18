CPMAddPackage( NAME GTest
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG v1.17.0
)

enable_testing()

add_executable( test_${PROJECT_NAME} )

list( APPEND SERVER_TEST_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/
)

list( APPEND SERVER_TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/test.cpp
)


target_sources( test_${PROJECT_NAME}
    PRIVATE
    ${SERVER_TEST_SOURCES}
    ${SERVER_SOURCES}
)

target_include_directories( test_${PROJECT_NAME}
    PRIVATE
    ${SERVER_TEST_INCLUDES}
    ${SERVER_INCLUDES}
)

target_compile_features( test_${PROJECT_NAME}
    PRIVATE
    cxx_std_20
)

target_link_libraries( test_${PROJECT_NAME}
    PRIVATE
    gtest_main
    gtest
    ${SERVER_LIBS}
)

include(GoogleTest)
gtest_discover_tests(test_${PROJECT_NAME})