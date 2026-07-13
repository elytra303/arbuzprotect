cmake_minimum_required(VERSION 3.10)
project(ArbuzProtectLauncher)

set(CMAKE_CXX_STANDARD 17)

# Исходники твоего приложения
set(SOURCES
    main.cpp
)

# Исходники Dear ImGui
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_win32.cpp
    imgui/backends/imgui_impl_dx11.cpp
)

# Создаем исполняемый файл (WIN32 скрывает консоль при запуске)
add_executable(ArbuzLauncher WIN32 ${SOURCES} ${IMGUI_SOURCES})

# Добавляем пути к заголовочным файлам
target_include_directories(ArbuzLauncher PRIVATE
    imgui
    imgui/backends
)

# Линкуем системные библиотеки Windows
target_link_libraries(ArbuzLauncher PRIVATE
    d3d11
    urlmon
)