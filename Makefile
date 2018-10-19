ifeq ($(OS),Windows_NT)
	rm=del /f /q
	target=nn.exe
	objdir=wobj
	PWD=$(shell cd)
	mkdir=if not exist "$1" mkdir "$1"
	convert=$(dir $(subst /,\,$1))
	suppress=> nul
else
	rm=rm -rf
	target=nn
	objdir=lobj
	PWD=$(shell pwd)
	mkdir=mkdir -p $1
	convert=$(dir $1)
	suppress=> /dev/null
endif

LDLIBS=-lpthread -lstdc++fs

INCLUDEDIR=$(PWD)/src
INCLUDEFLAGS=$(patsubst %, -I%, $(realpath $(INCLUDEDIR)))

CXXFLAGS =-std=c++17 $(INCLUDEFLAGS)
CXXDFLAGS=-g -O0 -DDEBUG
CXXRFLAGS=-O2

folders=$(sort $(dir $(wildcard src/*/)))
cpp=$(foreach var,$(folders),$(wildcard $(var)*.cpp))
obj=$(cpp:.cpp=.o)
obj:=$(patsubst %,$(objdir)/%,$(obj))

.PHONY: all clean debug release print

all: debug

debug: CXXFLAGS += $(CXXDFLAGS)
debug: $(target)

release: CXXFLAGS += $(CXXRFLAGS)
release: $(target)

$(target): $(obj)
	@echo [$(CXX)] $@
	@$(CXX) $(LDFLAGS) -o $@ $(obj) $(LDLIBS)
	
$(objdir)/%.o: %.cpp
	@echo [$(CXX)] $^
	@$(call mkdir,$(call convert,$@))
	@$(CXX) $(CXXFLAGS) -c $^ -o $@
	
clean:
	$(rm) $(call convert,$(obj)) $(target)
	
print:
	@echo $(folders)
	@echo $(cpp)
