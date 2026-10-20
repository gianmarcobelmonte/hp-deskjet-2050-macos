# hpcups (HPLIP 3.26.6) trimmed to the PCL3/PCL3GUI/PCL3GUI2 languages and
# built for macOS against the system CUPS. No external dependencies.

SRC = src/hpcups
CXX = clang++
CC  = clang
ARCHS = -arch x86_64 -arch arm64 -mmacosx-version-min=11.0
DEFS = -DHPCUPS_MINIMAL -DDISABLE_IMAGEPROCESSOR -DAPDK_LITTLE_ENDIAN \
       -DHAVE_STDINT_H -DCONFDIR=\"/etc/hp\" -DCUPS_FILTER_PATH=\"/usr/libexec/cups/filter\"
CPPFLAGS = -I$(SRC) -Isrc/common $(DEFS)
CXXFLAGS = $(ARCHS) -O2 -Wno-deprecated-declarations -Wno-write-strings -Wno-format-security
CFLAGS   = $(ARCHS) -O2 -Wno-deprecated-declarations
LDLIBS   = -lcups -lcupsimage -lz

CXX_SOURCES = HPCupsFilter.cpp Compressor.cpp Mode2.cpp Mode3.cpp Mode9.cpp \
	Mode10.cpp ModeDeltaPlus.cpp ErnieFilter.cpp EncapsulatorFactory.cpp \
	Encapsulator.cpp Pcl3.cpp Pcl3Gui.cpp Pcl3Gui2.cpp Job.cpp Pipeline.cpp \
	Processor.cpp RasterSender.cpp ColorMatcher.cpp Halftoner.cpp Scaler.cpp \
	dj400ColorMaps.cpp dj600ColorMaps.cpp dj970ColorMaps.cpp dj8xxColorMaps.cpp \
	dj4100ColorMaps.cpp BreakTables.cpp Utils.cpp SystemServices.cpp dbuscomm.cpp

OBJS = $(addprefix build/,$(CXX_SOURCES:.cpp=.o)) build/hplip_utils.o

all: hpcups usbprobe

hpcups: $(OBJS)
	$(CXX) $(ARCHS) -o build/$@ $(OBJS) $(LDLIBS)

usbprobe: tools/usbprobe.c | build
	$(CC) $(ARCHS) -O2 -o build/$@ $< -framework IOKit -framework CoreFoundation

build/%.o: $(SRC)/%.cpp | build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

build/hplip_utils.o: src/common/hplip_utils.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: all hpcups usbprobe clean
