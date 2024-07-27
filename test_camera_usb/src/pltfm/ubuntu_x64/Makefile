CPP := g++-11

CPPFLAGS := -std=c++20
CPPFLAGS += -mavx
#CPPFLAGS += -O3
#CPPFLAGS += -DNDEBUG

EXE := test_camera_usb

SDL2   := `sdl2-config --cflags --libs`
USB    := -pthread `pkg-config --libs --cflags libusb-1.0` -ljpeg

LDFLAGS := $(SDL2) $(USB)

ROOT := ../../..

BUILD := $(ROOT)/build/sdl

OUT := $(BUILD)/$(EXE)

SRC := test_camera_usb_main.cpp

#****************

build:
	$(CPP) $(CPPFLAGS) -o $(OUT) $(SRC) $(LDFLAGS)


run: build
	$(OUT)


clean:
	rm -rfv $(BUILD)/*


setup:
	mkdir -p $(BUILD)