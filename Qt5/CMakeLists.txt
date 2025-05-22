cmake_minimum_required(VERSION 3.16)
project(SkiaQtExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Qt6
find_package(Qt5 COMPONENTS OpenGL Widgets REQUIRED)

# --- Skia
# Set SKIA_DIR or use system-wide Skia
if(NOT SKIA_DIR)
    message(FATAL_ERROR "Please set SKIA_DIR to your Skia build directory (e.g. ~/skia/out/Static)")
endif()

include_directories(${SKIA_DIR})
link_directories(${SKIA_DIR}/out/Release) # Or wherever your Skia .a/.lib files are

# List Skia libs you need (example for static build)
set(SKIA_LIBS
    skia
    # Add any other Skia deps as needed (freetype, harfbuzz, etc.)
)

add_executable(SkiaQtExample
    SkiaQtExample.cpp
    SkiaGLWidget.cpp
    SkiaGLWidget.h
)

target_link_libraries(SkiaQtExample
    Qt5::OpenGL
    Qt5::Widgets
    ${SKIA_LIBS}
    # Add more (pthread, dl, etc.) if required by Skia on your platform
    GL
    webp
    webpdemux
    freetype
    jpeg
    png
    z
)