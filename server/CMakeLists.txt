add_executable(syslogkit-server src/ServerMain.cc)

target_link_libraries(syslogkit-server PRIVATE syslogkitbase)

if(WIN32)
    target_link_libraries(syslogkit-server PRIVATE ws2_32)
endif()