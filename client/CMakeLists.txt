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