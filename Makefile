CC=gcc
CXX=g++
BUILD_DIR=out
OBJ_DIR=out/obj
SRC_DIR=src
TARGET=$(BUILD_DIR)/nn.exe

ifeq ($(OS),Windows_NT) 
else #Linux machine
	TARGET=$(BUILD_DIR)/nn
	OBJ_DIR=out/obj-l
	#SFLAGS=
endif

INCLUDE_FLAGS=-I./src
LIBRARY_FLAGS=-lstdc++fs
CFLAGS=-std=c++17 -Wall -fdiagnostics-color=always
FFLAGS=$(CFLAGS) $(INCLUDE_FLAGS) $(LIBRARY_FLAGS)

SRC=$(wildcard $(SRC_DIR)/**/*.cpp)
OBJ=$(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: debug

debug: CFLAGS+=-DDEBUG -g -O0
debug: $(TARGET)

release: CFLAGS+=-O3
release: $(TARGET)
	
$(TARGET): $(OBJ)
	@echo Making $(TARGET) from $(OBJ)...
	@$(CXX) -o $@ $^ $(FFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo Making $@ from $<...
	@$(shell mkdir -p $(dir $@))
	@$(CXX) -c -o $@ $< $(FFLAGS)

.PHONY: clean

clean:
	@$(shell find . -type f -name '*.o' -delete)