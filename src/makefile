# Il peut �tre convivial de mettre l'affichage d'une aide comme
# premi�re cible.

help:
	@echo
	@echo "make lib      <-- Compile la lib."
	@echo "make build    <-- Compile tout."
	@echo "make clean    <-- Fais le m�nage."
	@echo "make install  <-- Compile et installe"
	@echo "make dist     <-- Fais un .tar.gz du projet"
	@echo

# Posons quelques macros

PROJECT_NAME = sparkle
VERSION      = 001

ARCHIVE_DIR_NAME = $(PROJECT_NAME)-$(VERSION)
ARCHIVE_NAME     = $(ARCHIVE_DIR_NAME).tar.gz

FILES_TO_BE_ARCHIVED = *.hpp *.cpp Makefile

INCLUDE_PATH = -I. -I../thirdparty/ -I./tools/ -I/usr/local/include/freetype2

#doesn't support -std=c++14 when using "g++"
COMPILO     =  ~/gcc-5.4.0/bin/g++
CXXFLAGS      =  -Wall   $(INCLUDE_PATH) -Wno-deprecated-declarations
CXXFLAGS_COOK  = $(CXXFLAGS) -fPIC -shared
LIBS        = -L. -lsparkle

INSTALL_PREFIX      = ./LocalInstall

INSTALL_BIN_DIR     = $(INSTALL_PREFIX)/bin
INSTALL_LIB_DIR     = $(INSTALL_PREFIX)/lib
INSTALL_INCLUDE_DIR = $(INSTALL_PREFIX)/include/sparkle

INSTALL_DIRS =					\
	$(INSTALL_BIN_DIR)			\
	$(INSTALL_LIB_DIR)			\
	$(INSTALL_INCLUDE_DIR)

# Pour l'instant, aucun .hpp n'est mis dans les d�pendances.... on
# devrait, mais c'est difficile (car les .hpp peuvent s'inclure les
# uns les autres).
########################################################
#SHELL := /bin/sh
#CC := ~/gcc-5.4.0/bin/g++
#CXX := ~/gcc-5.4.0/bin/g++

COMPILER_OPTIONS := -m64 -Wall -Wextra -Wshadow -Werror -pedantic -Iinclude     -fdiagnostics-color=auto -lsqlite3 -fno-builtin-qsort -lglfw -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo -Wno-deprecated-declarations
#INCLUDE_PATH = -I.

CXX_DEFINES = -DSHADERS_DIR=\"/Users/TACHER/sparklepp/src/shaders\" -DUSE_GLEW

#CFLAGS := -std=c11 $(COMPILER_OPTIONS)
#CXXFLAGS := -std=c++14 -Weffc++ $(COMPILER_OPTIONS) $(INCLUDE_PATH)

DEBUGFLAGS := -g -O0 -D _DEBUG
RELEASEFLAGS := -O2 -D NDEBUG
##########################################################

# Fabrication des .o (lib)

app.o :	app.cc
			$(COMPILO) $(CXX_DEFINES)  -c -std=c++14  $(CXXFLAGS_COOK) app.cc

events.o : events.cc
			$(COMPILO) $(CXX_DEFINES) -c -std=c++14 $(CXXFLAGS_COOK) events.cc

opengl.o : opengl.cc
			$(COMPILO) $(CXX_DEFINES) -c -std=c++14 $(CXXFLAGS_COOK) opengl.cc

scene.o : scene.cc
			$(COMPILO) $(CXX_DEFINES) -c -std=c++14 $(CXXFLAGS_COOK) scene.cc

append_consume_buffer.o : ./api/append_consume_buffer.cc
			$(COMPILO) $(CXX_DEFINES) -c -std=c++14 $(CXXFLAGS_COOK) ./api/append_consume_buffer.cc

gpu_particle.o : ./api/gpu_particle.cc
			$(COMPILO) $(CXX_DEFINES) -c -std=c++14 $(CXXFLAGS_COOK) ./api/gpu_particle.cc

random_buffer.o : ./api/random_buffer.cc
			$(COMPILO) $(CXX_DEFINES) -c -std=c++14 $(CXXFLAGS_COOK) ./api/random_buffer.cc

vector_field.o : ./api/vector_field.cc
			$(COMPILO) $(CXX_DEFINES) -c -std=c++14 $(CXXFLAGS_COOK) ./api/vector_field.cc

noise.o : ./api/noise.cc
			$(COMPILO) $(CXX_DEFINES) -c -std=c++14 $(CXXFLAGS_COOK) ./api/noise.cc

# Fabrication des .o (hors lib)

main.o : main.cc
			$(COMPILO) $(CXX_DEFINES) -c -std=c++14 $(CXXFLAGS_COOK) main.cc

# Fabrication de la lib

libsparkle.so : app.o events.o opengl.o scene.o append_consume_buffer.o gpu_particle.o random_buffer.o vector_field.o noise.o
	$(COMPILO) -o libsparkle.so -shared -lglfw3  -lFreetype -lGlew -framework Cocoa -framework OpenGL -framework Glut -framework IOKit -framework CoreVideo  app.o events.o opengl.o scene.o append_consume_buffer.o gpu_particle.o random_buffer.o vector_field.o noise.o

# Fabrication de l'ex�cutable

test.bin : main.o libsparkle.so
	$(COMPILO) -o   test.bin main.o  -lsoil -lglfw -lglfw3  -lGlew -framework Cocoa -framework OpenGL -framework Glut -framework IOKit -framework CoreVideo $(LIBS)

# Les cibles de l'aide

lib : libsparkle.so

build : test.bin

clean :
	@rm -f *~ *.o *.so *.bin
	@rm -rf $(ARCHIVE_DIR_NAME) $(ARCHIVE_NAME)
	@echo
	@echo "Cleaned up."

install : test.bin libcook.so
	@mkdir -p $(INSTALL_DIRS)
	@cp test.bin  $(INSTALL_BIN_DIR)
	@cp libcook.so $(INSTALL_LIB_DIR)
	@cp cook*.hpp  $(INSTALL_INCLUDE_DIR)
	@echo "File installed in " $(INSTALL_BIN_DIR) $(INSTALL_LIB_DIR) $(INSTALL_INCLUDE_DIR)

dist :
	@rm -rf $(ARCHIVE_DIR_NAME) $(ARCHIVE_NAME)
	@mkdir $(ARCHIVE_DIR_NAME)
	@cp $(FILES_TO_BE_ARCHIVED) $(ARCHIVE_DIR_NAME)
	@tar zcf $(ARCHIVE_NAME) $(ARCHIVE_DIR_NAME)
	@rm -rf $(ARCHIVE_DIR_NAME)
	@echo "File" $(ARCHIVE_NAME) " generated."
