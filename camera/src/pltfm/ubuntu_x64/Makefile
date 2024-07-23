GPP := g++-11 -std=c++20 -mavx

#GPP += -O3
#GPP += -DNDEBUG

GPP += -DIMAGE_READ
#GPP += -DIMAGE_WRITE
GPP += -DALLOC_COUNT

NO_FLAGS := 
SDL2 := `sdl2-config --cflags --libs`
OPENGL := -lGL -ldl

ALL_LFLAGS := $(SDL2) $(OPENGL)

root       := ../../../..

camera := $(root)/camera
build  := $(camera)/build/ubuntu
src    := $(camera)/src

pltfm := $(src)/pltfm/ubuntu_x64




libs := $(root)/libs

exe := camera

program_exe := $(build)/$(exe)


#*** libs ***

alloc_type := $(libs)/alloc_type
input      := $(libs)/input
image      := $(libs)/image
qsprintf   := $(libs)/qsprintf
span       := $(libs)/span
stb_image  := $(libs)/stb_image
usb        := $(libs)/usb
util       := $(libs)/util

sdl        := $(libs)/sdl

#************


#*** imgui ***

imgui := $(src)/imgui_sdl_ogl
imgui_options_h := $(imgui)/imgui_options.hpp

imgui_include_h := $(imgui)/imgui_include.hpp
imgui_include_h += $(imgui_options_h)

#**************


#*** libs/util ***

types_h        := $(util)/types.hpp
stopwatch_h    := $(util)/stopwatch.hpp
stack_buffer_h := $(util)/stack_buffer.hpp

numeric_h := $(util)/numeric.hpp
numeric_h += $(types_h)

#************


#*** alloc_type ***

alloc_type_h := $(alloc_type)/alloc_type.hpp
alloc_type_h += $(types_h)

alloc_type_c := $(alloc_type)/alloc_type.cpp
alloc_type_c += $(alloc_type_h)

#*************


#*** memory_buffer ***

memory_buffer_h := $(util)/memory_buffer.hpp
memory_buffer_h += $(alloc_type_h)

#***********


#*** stb_image ***

stb_image_h        := $(stb_image)/stb_image.h
stb_image_resize_h := $(stb_image)/stb_image_resize2.h

stb_image_options_h := $(stb_image)/stb_image_options.hpp
stb_image_options_h += $(stb_image_h)
stb_image_options_h += $(stb_image_resize_h)

#**************


#*** qsprintf ***

qsprintf_h := $(qsprintf)/qsprintf.hpp

qsprintf_c := $(qsprintf)/qsprintf.cpp

#***********


#*** span ***

span_h := $(span)/span.hpp
span_h += $(memory_buffer_h)
span_h += $(stack_buffer_h)
span_h += $(qsprintf_h)

span_c := $(span)/span.cpp
span_c += $(span_h)

#************


#*** input ***

input_h := $(input)/input.hpp
input_h += $(types_h)
input_h += $(input)/keyboard_input.hpp
input_h += $(input)/mouse_input.hpp
input_h += $(input)/controller_input.hpp


input_state_h := $(input)/input_state.hpp
input_state_h += $(input_h)

#**************


#*** image ***

image_h := $(image)/image.hpp
image_h += $(span_h)

image_c := $(image)/image.cpp
image_c += $(image_h)
image_c += $(numeric_h)
image_c += $(stb_image_h)
image_c += $(stb_image_resize_h)

#*************


#*** sdl ***

sdl_include_h := $(sdl)/sdl_include.hpp
sdl_include_h += $(input_h)
sdl_include_h += $(image_h)

sdl_input_c := $(sdl)/sdl_input.cpp
sdl_input_c += $(sdl_include_h)
sdl_input_c += $(input_state_h)
sdl_input_c += $(numeric_h)

#**********


#*** usb ***

camera_usb_h := $(usb)/camera_usb.hpp
libuvc_h     := $(usb)/libuvc2.hpp

camera_usb_c := $(usb)/camera_usb.cpp
camera_usb_c += $(camera_usb_h)
camera_usb_c += $(libuvc_h)

#**********


#*** input_display ***

input_display := $(src)/input_display

input_display_h := $(input_display)/input_display.hpp
input_display_h += $(input_h)
input_display_h += $(image_h)

#***************


#*** diagnostics ***

diagnostics := $(src)/diagnostics

diagnostics_h := $(diagnostics)/diagnostics.hpp
diagnostics_c := $(diagnostics)/diagnostics.cpp

#***************


#*** camera app ***


#******************


#*** main cpp ***

main_c := $(pltfm)/camera_main.cpp
main_o := $(build)/main.o
obj    := $(main_o)

main_dep := $(imgui_include_h)
main_dep += $(input_display_h)
main_dep += $(sdl_include_h)
main_dep += $(stopwatch_h)
main_dep += $(alloc_type_c)
main_dep += $(image_c)
main_dep += $(qsprintf)
main_dep += $(span_c)
main_dep += $(stb_image_options_h)
main_dep += $(sdl_input_c)
main_dep += $(camera_usb_c)
main_dep += $(diagnostics_h)
main_dep += $(diagnostics_c)

#****************


#*** imgui cpp ***

imgui_c := $(imgui)/imgui_o.cpp
imgui_o := $(build)/imgui.o
obj     += $(imgui_o)

imgui_dep := $(imgui_options_h)

#****************


#*** input_display cpp ***

input_display_c := $(input_display)/input_display.cpp
input_display_o := $(build)/input_display.o
obj += $(input_display_o)

input_display_dep := $(input_display_h)
input_display_dep += $(qsprintf_h)
input_display_dep += $(span_h)

#***************


#*** app ***


$(main_o): $(main_c) $(main_dep)
	@echo "\n  main"
	$(GPP) -o $@ -c $< $(SDL2) $(OPENGL)


$(imgui_o): $(imgui_c) $(imgui_dep)
	@echo "\n  imgui"
	$(GPP) -o $@ -c $< $(SDL2) $(OPENGL)


$(input_display_o): $(input_display_c) $(input_display_dep)
	@echo "\n  input_display"
	$(GPP) -o $@ -c $< $(NO_FLAGS)

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