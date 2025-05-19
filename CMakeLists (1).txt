cmake_minimum_required(VERSION 3.16)
project(SkiaQtExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Qt6
find_package(Qt6 COMPONENTS Widgets OpenGL REQUIRED)

# --- Skia
# Set SKIA_DIR or use system-wide Skia
set(SKIA_DIR "/path/to/skia")  # Change this to your SkIA build path

include_directories(${SKIA_DIR}/include)
link_directories(${SKIA_DIR}/out/Static) # Or wherever your Skia .a/.lib files are

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
    Qt6::Widgets
    Qt6::OpenGL
    ${SKIA_LIBS}
    # Add more (pthread, dl, etc.) if required by Skia on your platform
)