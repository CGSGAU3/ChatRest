include(FetchContent)

FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.23.1
)

FetchContent_MakeAvailable(cpp-httplib)

CPMAddPackage( NAME SQLiteCpp
    GIT_REPOSITORY "https://github.com/SRombauts/SQLiteCpp.git"
    GIT_TAG 3.3.3
) 

CPMAddPackage( NAME spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
    GIT_TAG v1.15.3
)

CPMAddPackage( NAME nlohmann_json
    GIT_REPOSITORY "https://github.com/nlohmann/json.git"
    GIT_TAG v3.12.0
)

list( APPEND SERVER_LIBS
    httplib
    SQLiteCpp
    sqlite3
    nlohmann_json
    spdlog::spdlog
)