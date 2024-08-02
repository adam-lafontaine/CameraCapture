GPP := g++-11 -std=c++20 -mavx

GPP += -DIMAGE_READ

NO_FLAGS :=
ALL_LFLAGS :=


root   := /home/adam/Repos/CameraCapture

tools := $(root)/camera/tools

src := $(tools)/image

build := $(tools)/build/image

libs := $(root)/libs

exe := image_to_code

program_exe := $(build)/$(exe)



#*** main cpp ***

main_c := $(src)/image_to_code_main.cpp
main_o := $(build)/main.o
obj := $(main_o)

main_dep :=

#************


$(main_o): $(main_c) $(main_dep)
	@echo "\n  main"
	$(GPP) -o $@ -c $< $(ALL_LFLAGS)


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