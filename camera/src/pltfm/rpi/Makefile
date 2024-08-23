GPP := g++-11

GPP += -std=c++20
GPP += -mavx
#GPP += -O3
GPP += -DNDEBUG

NO_FLAGS := 
SDL2   := `sdl2-config --cflags --libs`
OPENGL := -lGL -ldl
USB    := -pthread `pkg-config --libs --cflags libusb-1.0` -ljpeg

ALL_LFLAGS := $(SDL2) $(OPENGL) $(USB)

root       := ../../../..

camera := $(root)/camera
build  := $(camera)/build/rpi
src    := $(camera)/src
pltfm  := $(src)/pltfm/ubuntu_x64

exe := camera

program_exe := $(build)/$(exe)

#**************


#*** main cpp ***

main_c := $(pltfm)/camera_main.cpp
main_o := $(build)/main.o
obj    := $(main_o)

#****************


#*** imgui cpp ***

imgui_c := $(pltfm)/imgui_o.cpp
imgui_o := $(build)/imgui.o
obj     += $(imgui_o)

#****************


#*** app ***


$(main_o):
	@echo "\n  main"
	$(GPP) -o $@ -c $(main_c) $(ALL_LFLAGS)


$(imgui_o):
	@echo "\n  imgui"
	$(GPP) -o $@ -c $(imgui_c) $(SDL2) $(OPENGL)

#**************


$(program_exe): $(obj)
	@echo "\n  program_exe"
	$(GPP) -o $@ $+ $(ALL_LFLAGS)


build: $(program_exe)


run: build
	$(program_exe)
	@echo "\n"


clean:
	rm -rfv $(build)/*

setup:
	mkdir -p $(build)