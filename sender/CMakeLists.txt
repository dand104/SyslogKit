cmake_minimum_required(VERSION 3.30)
project(SyslogKitSender LANGUAGES CXX)

add_executable(SyslogKitSender
        sender.cxx
)

target_link_libraries(SyslogKitSender PRIVATE
        syslogkit_lib
)

if(WIN32)
    target_link_libraries(SyslogKitSender PRIVATE ws2_32)
endif()

include(GNUInstallDirs)
install(TARGETS SyslogKitSender
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)