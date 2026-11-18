@echo off
REM build.bat - builds without make. Usage: build.bat   (then: library.exe  /  run_tests.exe)
set FLAGS=-std=c11 -O2 -Wall -Wextra -pedantic -D__USE_MINGW_ANSI_STDIO=1 -fno-builtin-snprintf -Iinclude
set SRC=src\vec.c src\rng.c src\heap.c src\model.c src\datagen.c src\divide_conquer.c src\string_match.c src\greedy.c src\dp.c src\backtrack.c src\graph.c src\library_engine.c src\io.c src\bench.c src\timer.c src\scenarios.c
gcc %FLAGS% %SRC% src\main.c -o library.exe -lm || exit /b 1
gcc %FLAGS% %SRC% tests\test_main.c -o run_tests.exe -lm || exit /b 1
echo Build OK: library.exe and run_tests.exe
