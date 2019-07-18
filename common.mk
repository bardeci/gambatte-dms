
CFLAGS = $(DEFINES) $(INCLUDES) $(OPT_FLAGS) -std=gnu11 
CXXFLAGS = $(DEFINES) $(INCLUDES) $(OPT_FLAGS) -std=gnu++11 
LDFLAGS = -Wl,--start-group -lSDL -lSDL_image -lpng -ljpeg -lSDL_mixer -logg -lvorbisidec -lmikmod -lmodplug -lm -pthread -lz -lstdc++ $(EXTRA_LDFLAGS) -Wl,--end-group

# Redream (main engine)
OBJS =  \
	libgambatte/src/bitmap_font.o \
	libgambatte/src/bootloader.o \
	libgambatte/src/cpu.o \
	libgambatte/src/gambatte.o \
	libgambatte/src/initstate.o \
	libgambatte/src/interrupter.o \
	libgambatte/src/interruptrequester.o \
	libgambatte/src/loadres.o \
	libgambatte/src/memory.o \
	libgambatte/src/sound.o \
	libgambatte/src/state_osd_elements.o \
	libgambatte/src/statesaver.o \
	libgambatte/src/tima.o \
	libgambatte/src/video.o \
	libgambatte/src/mem/cartridge.o \
	libgambatte/src/mem/memptrs.o \
	libgambatte/src/mem/pakinfo.o \
	libgambatte/src/mem/rtc.o \
	libgambatte/src/sound/channel1.o \
	libgambatte/src/sound/channel2.o \
	libgambatte/src/sound/channel3.o \
	libgambatte/src/sound/channel4.o \
	libgambatte/src/sound/duty_unit.o \
	libgambatte/src/sound/envelope_unit.o \
	libgambatte/src/sound/length_counter.o \
	libgambatte/src/video/ly_counter.o \
	libgambatte/src/video/lyc_irq.o \
	libgambatte/src/video/next_m0_time.o \
	libgambatte/src/video/ppu.o \
	libgambatte/src/video/sprite_mapper.o
	
ifeq ($(NOZIP), YES)
OBJS += libgambatte/src/file/file.o
else
OBJS += libgambatte/src/file/file_zip.o libgambatte/src/file/unzip/ioapi.o libgambatte/src/file/unzip/unzip.o
endif
	
OBJS +=	gambatte_sdl/src/audiosink.o \
	gambatte_sdl/src/blitterwrapper.o \
	gambatte_sdl/src/parser.o \
	gambatte_sdl/src/sdlblitter.o \
	gambatte_sdl/src/str_to_sdlkey.o \
	gambatte_sdl/src/usec.o \
	gambatte_sdl/src/gambatte_sdl.o \
	gambatte_sdl/SFont.o \
	gambatte_sdl/menu.o \
	gambatte_sdl/libmenu.o \
	gambatte_sdl/scaler.o \
	common/adaptivesleep.o \
	common/resample/src/chainresampler.o \
	common/resample/src/i0.o \
	common/resample/src/kaiser50sinc.o \
	common/resample/src/kaiser70sinc.o \
	common/resample/src/makesinckernel.o \
	common/resample/src/resamplerinfo.o \
	common/resample/src/u48div.o \
	common/rateest.o \
	common/skipsched.o \
	common/videolink/rgb32conv.o \
	common/videolink/vfilterinfo.o
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< 
	
.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $< 
	
all: executable

gambatte_sdl/menu.o: builddate

builddate:
	echo "#define BUILDDATE \"$$(date +'%Y%m%d-%H%M%S')"\" > ./gambatte_sdl/builddate.h

executable: $(OBJS)
	$(CC) -o $(OUTPUTNAME) $(OBJS) $(CFLAGS) $(LDFLAGS)

clean:
	rm $(OBJS) $(OUTPUTNAME)
