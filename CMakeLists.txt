cmake_minimum_required(VERSION 3.10)
project(ArbuzProtectLauncher)

set(CMAKE_CXX_STANDARD 17)

# Принудительно заставляем CMake сохранять бинарник в корень папки сборки, а не в подпапки
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(SOURCES
    main.cpp
)

set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_dx11.cpp
)

add_executable(ArbuzLauncher WIN32 ${SOURCES} ${IMGUI_SOURCES})

target_include_directories(ArbuzLauncher PRIVATE
    imgui
    imgui/backends
)

target_link_libraries(ArbuzLauncher PRIVATE
    d3d11
    urlmon
)
