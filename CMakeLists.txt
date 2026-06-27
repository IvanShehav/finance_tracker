cmake_minimum_required(VERSION 3.20)

project(FinanceTrackerQt VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_executable(FinanceTracker
    src/main.cpp
    src/Models.cpp
    src/Models.h
    src/FinanceManager.cpp
    src/FinanceManager.h
    src/ChartWidgets.cpp
    src/ChartWidgets.h
    src/InsiderService.cpp
    src/InsiderService.h
    src/MarketService.cpp
    src/MarketService.h
    src/MainWindow.cpp
    src/MainWindow.h
)

target_link_libraries(FinanceTracker PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

if (MSVC)
    target_compile_options(FinanceTracker PRIVATE /utf-8)
endif()

if (WIN32)
    set_target_properties(FinanceTracker PROPERTIES WIN32_EXECUTABLE TRUE)

    foreach(runtime_dll libssl-3-x64.dll libcrypto-3-x64.dll)
        find_file(${runtime_dll}_PATH
            NAMES ${runtime_dll}
            PATHS "${CMAKE_PREFIX_PATH}/bin" "C:/msys64/ucrt64/bin"
            NO_DEFAULT_PATH
        )
        if (${runtime_dll}_PATH)
            add_custom_command(TARGET FinanceTracker POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${${runtime_dll}_PATH}"
                    "$<TARGET_FILE_DIR:FinanceTracker>"
            )
        endif()
    endforeach()
endif()
