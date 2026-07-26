CXX=g++
SDKROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.5.sdk
CFLAGS=-std=c++17 -stdlib=libc++ -Ofast -march=native -flto -fno-signed-zeros -fno-trapping-math -funroll-loops -Wno-deprecated -I/usr/local/include -I. -Isrc/ -Ithird_party -isysroot $(SDKROOT)
LDFLAGS=-Wl,-search_paths_first -Wl,-headerpad_max_install_names -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo -framework CoreFoundation -lraylib -Lthird_party/raylib/ -lbox3d -Lthird_party/box3d/
EXEC=main

all: 
	$(CXX) $(CFLAGS) $(LDFLAGS) src/main.cpp -o $(EXEC)

clean:
	rm -f $(EXEC) *.o *.gch src/*.o src/*.gch
