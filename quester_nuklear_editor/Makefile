CFLAGS += -std=c99 -ggdb

SRC = main.c
OBJ = $(SRC:.c=.o)

BIN = quester
ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
LIBS = -lglfw3 -lopengl32 -lm -lGLU32 -lGLEW32
else
	UNAME_S := $(shell uname -s)
	GLFW3 := $(shell pkg-config --libs glfw3)
	ifeq ($(UNAME_S),Darwin)
		LIBS := $(GLFW3) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm -lGLEW -L/usr/local/lib
	else
		LIBS = $(GLFW3) -lGL -lm -lGLU -lGLEW
	endif
endif

all:
	rm -f $(BIN)
	$(CC) $(SRC) $(CFLAGS) -I../ -o $(BIN) $(LIBS)
