project(SyslogKitCore)
include(FetchContent)

FetchContent_Declare(
        sqlite3
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3470200.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(sqlite3)
set(SqliteLibrary ${sqlite3_SOURCE_DIR}/sqlite3.c ${sqlite3_SOURCE_DIR}/sqlite3.h)

add_library(syslogkit_lib STATIC
        src/SyslogProto.cxx
        src/SyslogServer.cxx
        src/LogStorage.cxx
        inc/SyslogKit/SyslogProto.hxx
        inc/SyslogKit/SyslogServer.hxx
        inc/SyslogKit/LogStorage.hxx
        ${SqliteLibrary}
)

target_include_directories(syslogkit_lib PUBLIC inc ${sqlite3_SOURCE_DIR})

if(WIN32)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/inc" CORE_INC_DIR)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src" CORE_SRC_DIR)
    file(TO_NATIVE_PATH "${sqlite3_SOURCE_DIR}" SQLITE3_INC_DIR)
    file(TO_NATIVE_PATH "${sqlite3_SOURCE_DIR}/sqlite3.c" SQLITE3_C_PATH)

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/syslogkit_lib.props.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/syslogkit_lib.props"
        @ONLY
    )
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/syslogkit_lib.targets.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/syslogkit_lib.targets"
        @ONLY
    )
endif()

if(UNIX)
    target_link_libraries(syslogkit_lib PRIVATE pthread dl)
endif()
if(WIN32)
    target_link_libraries(syslogkit_lib PRIVATE ws2_32)
endif()