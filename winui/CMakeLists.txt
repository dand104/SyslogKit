cmake_minimum_required(VERSION 3.25)
project(SyslogKitWinUI NONE)

find_program(VSWHERE_PATH vswhere
        PATHS
        "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer"
        "$ENV{ProgramFiles}/Microsoft Visual Studio/Installer"
)

execute_process(
        COMMAND "${VSWHERE_PATH}" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild/**/Bin/MSBuild.exe
        OUTPUT_VARIABLE MSBUILD_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT EXISTS "${MSBUILD_PATH}")
    message(WARNING "MSBuild.exe was not found via vswhere: ${MSBUILD_PATH}")
endif()
message(STATUS "Found MSBuild: ${MSBUILD_PATH}")

find_program(NUGET_EXE nuget PATHS "$ENV{PATH}")

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Directory.Build.props.in"
        "${CMAKE_CURRENT_SOURCE_DIR}/Directory.Build.props"
        COPYONLY
)

set(SOLUTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../SyslogKit.slnx")
set(VCXPROJ_FILE "${CMAKE_CURRENT_SOURCE_DIR}/SyslogKitWin.vcxproj")

if(NOT CMAKE_BUILD_TYPE)
    set(BUILD_CONFIG "Debug")
else()
    set(BUILD_CONFIG "${CMAKE_BUILD_TYPE}")
endif()

if(NOT DEFINED BUILD_PLATFORM) # x64, ARM64, Win32
    set(BUILD_PLATFORM "x64")
endif ()

add_custom_target(winui_nuget_restore
        COMMAND ${CMAKE_COMMAND} -E echo "Restoring NuGet packages..."
        COMMAND nuget restore "${SOLUTION_FILE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_target(winui_app
        COMMAND "${MSBUILD_PATH}" "${VCXPROJ_FILE}"
        /p:Configuration=${BUILD_CONFIG}
        /p:Platform=${BUILD_PLATFORM}
        /p:RestorePackages=false
        /m
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Building WinUI3 app via MSBuild"
)

if(NOT MSIX_OUTPUT_DIR)
    set(MSIX_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif ()
add_custom_target(winui_msix
        COMMAND "${MSBUILD_PATH}" "${VCXPROJ_FILE}"
        /p:Configuration=${BUILD_CONFIG}
        /p:Platform=${BUILD_PLATFORM}
        /p:GenerateAppxPackageOnBuild=true
        /p:AppxPackageDir="${MSIX_OUTPUT_DIR}/"
        /p:AppxBundle=Never
        /p:UapAppxPackageBuildMode=SideloadOnly
        /p:AppxPackageSigningEnabled=false
        /p:RestorePackages=false
        /m
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Packaging MSIX for ${BUILD_CONFIG}|${BUILD_PLATFORM} -> ${MSIX_OUTPUT_DIR}"
)