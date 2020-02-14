include autogen.mk

LDLIBS=-lpthread -lstdc++fs

INCLUDEDIR=$(shell pwd)/src
INCLUDEFLAGS=$(patsubst %, -I%, $(realpath $(INCLUDEDIR)))

CXXFLAGS =-std=c++17 -Wall -Wextra $(INCLUDEFLAGS)
CXXDFLAGS=-g -O0 -DDEBUG
CXXRFLAGS=-O2

folders=$(sort $(dir $(shell find .)))
cpp=$(foreach var,$(folders),$(wildcard $(var)*.cpp))
obj=$(patsubst %,$(objdir)/%,$(cpp:.cpp=.o))

.PHONY: all clean debug release print generate

all: debug

debug: CXXFLAGS += $(CXXDFLAGS)
debug: $(target)

release: CXXFLAGS += $(CXXRFLAGS)
release: $(target)

profile: CXXFLAGS += -pg
profile: CXXFLAGS += $(CXXDFLAGS)
profile: LDFLAGS += -pg
profile: $(target)

$(target): $(obj)
	@echo [$(CXX)] $@
	@$(CXX) $(LDFLAGS) -o $@ $(obj) $(LDLIBS)
	
$(objdir)/%.o: %.cpp
	@echo [$(CXX)] $^
	@$(call mkdir,$(call convert,$@))
	@$(CXX) $(CXXFLAGS) -c $^ -o $@
	
autogen.mk: $(shell find . -name *.template*) 
	@echo [python3] Generating...
	@python3 src/autogeneration/nnasm/generate.py
	@touch autogen.mk
	
clean:
	rm $(call convert,$(obj)) $(target)
	rm $(shell find . -name *.generated*)
	rm autogen.mk
	
print:
	@echo $(folders)
	@echo $(cpp)
