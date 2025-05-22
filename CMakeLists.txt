cmake_minimum_required(VERSION 3.14)
project(SkiaQTExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ---- Find Qt ----
find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)

# ---- Skia ----
# You must set SKIA_DIR to your local Skia build directory, or edit these paths.
set(SKIA_DIR "" CACHE PATH "Path to your Skia build directory")
if(NOT SKIA_DIR)
    message(FATAL_ERROR "Please set SKIA_DIR to your Skia build directory (e.g. ~/skia/out/Static)")
endif()
include_directories(${SKIA_DIR})
link_directories(${SKIA_DIR})

# Pick Skia libraries you need - edit as required for your build.
set(SKIA_LIBS
    ${SKIA_DIR}/out/Release/libskia.a
    # Add any additional Skia or system libraries as needed.
    GL
    fontconfig
    webp
    webpdemux
    freetype
    jpeg
    png
    z
    pthread
    dl
)

# ---- Sources ----
set(SOURCES
    SkiaQTExample.cpp
)

add_executable(SkiaQTExample ${SOURCES})

target_link_libraries(SkiaQTExample
    Qt5::Widgets
    Qt5::OpenGL
    ${SKIA_LIBS}
)

# If you need to define SK_GL, SK_GANESH, etc, add:
# target_compile_definitions(SkiaQTExample PRIVATE SK_GL SK_GANESH)

# You may need to add additional include_directories or link_libraries depending on your Skia build.