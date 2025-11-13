list( APPEND SERVER_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/database
    ${CMAKE_CURRENT_LIST_DIR}/sha256/
    ${CMAKE_CURRENT_LIST_DIR}/server/
    ${CMAKE_CURRENT_LIST_DIR}/server/response_converter/
    ${CMAKE_CURRENT_LIST_DIR}/server/response_error_builder/
)

list( APPEND SERVER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/
    ${CMAKE_CURRENT_LIST_DIR}/database/database.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sha256/sha256.cpp
    ${CMAKE_CURRENT_LIST_DIR}/server/server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/server/response_converter/response_converter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/server/response_error_builder/response_error_builder.cpp
)