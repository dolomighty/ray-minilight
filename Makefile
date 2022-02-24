


PRE:=$(shell ./mk-headers.sh)
PRE:=$(shell ./mk-shell.sh)
SRC:=$(shell find -type f -name "*.c" -or -name "*.cpp" -or -name "*.asm")
HDR:=$(shell find -type f -name "*.h" -or -name "*.hpp")
RES:=$(shell find -type f -name "*.png")
DIR:=$(shell find -L -mindepth 1 -type d -not -wholename "*/.*" -printf "-I %P ")




CC=g++
LIBS    =`pkg-config --libs   sdl2` -lm
CPPFLAGS=`pkg-config --cflags sdl2` -fopenmp -Werror $(DIR)

# optim
CPPFLAGS+=-O3


.PHONY : all
all : main


.PHONY : run
run : main
	./$^ scenes/room.obj


OBS+=Camera.o
OBS+=Random.o
OBS+=RayTracer.o
OBS+=Scene.o
OBS+=SpatialIndex.o
OBS+=SurfacePoint.o
OBS+=Triangle.o
OBS+=V3f.o

OBS+=loop.o
OBS+=frame.o
OBS+=last.o
OBS+=M34.o
OBS+=globals.o
OBS+=hdr.o
OBS+=obj_import.o

OBS+=main.o
#main.o : $(SRC) $(HDR)

$(OBS) : $(SRC) $(HDR)


main : Makefile $(OBS)
	$(CC) $(CPPFLAGS) -o $@ $(OBS) $(LIBS)

#DYN+=draw_scene_gl.h
#draw_scene_gl.h : scenes/scene.obj obj2c.sh
#	./obj2c.sh scenes/scene.obj > $@

.PHONY : clean cl
clean cl :
	file * | awk '/ELF/ { gsub(/:.*/,"") ; print }' | xargs -r rm
	rm -fR deps.inc dyn

.PHONY : rebuild re
rebuild re : clean all

