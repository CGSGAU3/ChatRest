include(FetchContent)

FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.23.1
)

FetchContent_MakeAvailable(cpp-httplib)

list( APPEND SERVER_LIBS
    httplib
)