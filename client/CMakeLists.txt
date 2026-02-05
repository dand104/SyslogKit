find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        REQUIRED)
add_executable(SyslogKit
        src/ApplicationMain.cpp
        src/MainWindow.cpp
        src/MainWindow.hpp
)

set_target_properties(SyslogKit PROPERTIES
        WIN32_EXECUTABLE ON
        MACOSX_BUNDLE ON
)

target_link_libraries(SyslogKit PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Network
        syslogkitbase
)

set_target_properties(SyslogKit PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
)