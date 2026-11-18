# Makefile - Digital Library Search and Recommendation Engine (DAA Project 17), C version
#   Windows (MinGW):  mingw32-make        Linux/macOS:  make
#   Targets: all (default), test, demo, bench, clean
#
# -std=c11: gcc 6.x has no -std=c17 flag; C17 only fixed defects in C11, so the code is the same.
# -D__USE_MINGW_ANSI_STDIO=1: makes MinGW's printf support %lld (64-bit integers).
# -fno-builtin-snprintf: gcc's built-in snprintf check assumes the old Microsoft printf on Windows
#   and wrongly flags %lld; this makes it use the header's (correct) format rules instead.

ifeq ($(origin CC),default)
  CC = gcc
endif
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -pedantic -D__USE_MINGW_ANSI_STDIO=1 -fno-builtin-snprintf -Iinclude
LDLIBS  = -lm

LIB_SRC = src/vec.c src/rng.c src/heap.c src/model.c src/datagen.c src/divide_conquer.c \
          src/string_match.c src/greedy.c src/dp.c src/backtrack.c src/graph.c \
          src/library_engine.c src/io.c src/bench.c src/timer.c src/scenarios.c
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

$(APP): $(LIB_SRC) src/main.c $(HEADERS)
	$(CC) $(CFLAGS) $(LIB_SRC) src/main.c -o $(APP) $(LDLIBS)

$(TEST): $(LIB_SRC) tests/test_main.c $(HEADERS)
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_main.c -o $(TEST) $(LDLIBS)

test: $(TEST)
	"$(CURDIR)/$(TEST)"

demo: $(APP)
	"$(CURDIR)/$(APP)" --scenario all

bench: $(APP)
	"$(CURDIR)/$(APP)" --bench

clean:
	$(RM) $(APP) $(TEST)

.PHONY: all test demo bench clean
