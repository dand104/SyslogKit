project(SyslogKitCommon)

add_library(syslogkitbase STATIC
        src/SyslogProto.cc
        inc/SyslogKit/SyslogProto
)

target_include_directories(syslogkitbase PUBLIC inc)