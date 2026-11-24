# ============================================================================
# Makefile — Space Shooter (raylib)
#   make                 compila o jogo
#   make run             compila e executa
#   make RELEASE=1       versão otimizada (no Windows, sem janela de console)
#   make clean           apaga o executável
# ============================================================================

CC      ?= gcc
TARGET  := SpaceShooter
SRCS    := $(wildcard src/*.c)
HDRS    := $(wildcard src/*.h)

CFLAGS  := -std=c99 -Wall -Wextra -Isrc
LDFLAGS :=

ifeq ($(RELEASE),1)
    CFLAGS += -O2 -DNDEBUG
else
    CFLAGS += -O1 -g
endif

ifdef RAYLIB_PATH
    CFLAGS  += -I$(RAYLIB_PATH)/include -I$(RAYLIB_PATH)/src
    LDFLAGS += -L$(RAYLIB_PATH)/lib -L$(RAYLIB_PATH)/src
endif

# Bibliotecas de sistema que a raylib precisa em cada plataforma
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    LDLIBS ?= -lraylib -lopengl32 -lgdi32 -lwinmm -lm
    ifeq ($(RELEASE),1)
        LDFLAGS += -mwindows
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        LDLIBS ?= -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm
    else
        LDLIBS ?= -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    endif
endif

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET)

.PHONY: all run clean
