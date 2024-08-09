# The below assumes that there is a skia build in "./skia/", where the built shared
# libraries are in "./skia/out/Shared/".

CXXFLAGS=-DGR_GL_CHECK_ERROR=0 -DGR_GL_LOG_CALLS=0 -Wall  -I./skia -I/usr/include/SDL2/
LDFLAGS=-L./skia/out/Shared/ -lskparagraph -lskunicode_icu -lskia -lfreetype -lwebp -ljpeg -lwebpdemux -lpng -lz  -lSDL2 -lGL

default: SkiaSDLExample shape_text

%.%.cpp :
# The default implicit rule seems to be $(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@
