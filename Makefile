# Makefile - Digital Library Search and Recommendation Engine (DAA Project 17)
#   Windows (MinGW):  mingw32-make        Linux/macOS:  make
#   Targets: all (default), test, demo, bench, clean

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Iinclude

LIB_SRC = src/datagen.cpp src/divide_conquer.cpp src/string_match.cpp src/greedy.cpp \
          src/dp.cpp src/backtrack.cpp src/graph.cpp src/library_engine.cpp src/io.cpp \
          src/bench.cpp src/timer.cpp src/scenarios.cpp
HEADERS = $(wildcard include/*.h)

ifeq ($(OS),Windows_NT)
  EXE = .exe
  RM  = del /Q
else
  EXE =
  RM  = rm -f
endif

APP  = library$(EXE)
TEST = run_tests$(EXE)

all: $(APP) $(TEST)

$(APP): $(LIB_SRC) src/main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) src/main.cpp -o $(APP)

$(TEST): $(LIB_SRC) tests/test_main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) tests/test_main.cpp -o $(TEST)

test: $(TEST)
	"$(CURDIR)/$(TEST)"

demo: $(APP)
	"$(CURDIR)/$(APP)" --scenario all

bench: $(APP)
	"$(CURDIR)/$(APP)" --bench

clean:
	$(RM) $(APP) $(TEST)

.PHONY: all test demo bench clean
