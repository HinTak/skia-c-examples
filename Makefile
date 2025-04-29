# The below assumes that there is a skia build in "./skia/", where the built shared
# libraries are in "./skia/out/Shared/".

CXXFLAGS=-DGR_GL_CHECK_ERROR=0 -DGR_GL_LOG_CALLS=0 -Wall  -I./skia -I/usr/include/SDL2/ \
         -DSK_FONTMGR_FONTCONFIG_AVAILABLE
LDFLAGS=-L./skia/out/Shared/ -lskparagraph -lsvg -lskshaper -lskunicode_icu -lskunicode_core -lskia \
        -lfreetype -lwebp -ljpeg -lwebpdemux -lpng -lz  -lSDL2 -lGL -lX11



BINS=SkiaSDLExample \
 decode_everything \
 decode_png_main \
 ganesh_gl \
 ganesh_vulkan \
 path_main \
 shape_text \
 svg_renderer \
 write_text_to_png \
 write_to_pdf

default: $(BINS)

.phony: clean

clean:
	$(RM) $(BINS)

%.%.cpp :
# The default implicit rule seems to be $(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@
# It should be corrected to $(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
