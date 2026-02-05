project(SyslogKitCommon)

set(SQLITE_SOURCES
        vendor/sqlite3/sqlite3.c
        vendor/sqlite3/sqlite3.h
)

add_library(syslogkitbase STATIC
        src/SyslogProto.cc
        src/SyslogServer.cc
        src/LogStorage.cc
        inc/SyslogKit/SyslogProto.hxx
        inc/SyslogKit/SyslogServer.hxx
        inc/SyslogKit/LogStorage.hxx
        ${SQLITE_SOURCES}
)

target_include_directories(syslogkitbase PUBLIC inc vendor/sqlite3)
if(UNIX)
    target_link_libraries(syslogkitbase PRIVATE pthread dl)
endif()
if(WIN32)
    target_link_libraries(syslogkitbase PRIVATE ws2_32)
endif()