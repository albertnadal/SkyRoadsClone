CC = gcc

SDKROOT = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.5.sdk

CFLAGS = -std=c11 -Ofast -march=native -flto \
          -fno-signed-zeros -fno-trapping-math -funroll-loops \
          -Wno-deprecated \
          -I/usr/local/include -I. -Isrc -Ithird_party \
          -isysroot $(SDKROOT)

LDFLAGS = -Wl,-search_paths_first \
          -Wl,-headerpad_max_install_names \
          -framework OpenGL \
          -framework Cocoa \
          -framework IOKit \
          -framework CoreAudio \
          -framework CoreVideo \
          -framework CoreFoundation \
          -Lthird_party/raylib -lraylib \
          -Lthird_party/box3d -lbox3d

EXEC = main

SRC = \
	src/main.c \
	src/level.c \
	src/lane.c \
	src/tunnel.c

OBJ = $(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJ)

.PHONY: all clean