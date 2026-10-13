find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        REQUIRED)

include(GNUInstallDirs)

add_executable(SyslogKitClient
        src/ApplicationMain.cpp
        src/MainWindow.cpp
        src/MainWindow.hpp
        src/SyslogModel.cpp
        src/SyslogModel.hpp
        src/LogDetailDialog.cpp
        src/LogDetailDialog.hpp
)

set_target_properties(SyslogKitClient PROPERTIES
        WIN32_EXECUTABLE ON

        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_BUNDLE_NAME "SyslogKit"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.dand104.syslogkit"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

target_link_libraries(SyslogKitClient PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        syslogkit_lib
)

set_target_properties(SyslogKitClient PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
)

install(TARGETS SyslogKitClient
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)